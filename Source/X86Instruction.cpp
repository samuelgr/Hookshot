/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file X86Instruction.cpp
 *   Implementation of functionality for manipulating binary X86 instructions.
 **************************************************************************************************/

#include "X86Instruction.h"

#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

extern "C"
{
#include "xed/xed-interface.h"
}

#include <Infra/Core/TemporaryBuffer.h>

namespace Hookshot
{
  /// Captures the state of the current machine, based on its operating mode (i.e. 32-bit or
  /// 64-bit). This is a constant that corresponds to the currently-executing process mode.
#ifdef _WIN64
  static constexpr xed_state_t kXedMachineState = {XED_MACHINE_MODE_LONG_64, XED_ADDRESS_WIDTH_64b};
#else
  static constexpr xed_state_t kXedMachineState = {
      XED_MACHINE_MODE_LEGACY_32, XED_ADDRESS_WIDTH_32b};
#endif

#ifdef _WIN64
  /// Mask used to determine if a byte is a REX prefix of any form.
  static constexpr uint8_t kRexPrefixMask = 0xf0;

  /// Comparison value used to determine if a byte is a REX prefix of any form.
  static constexpr uint8_t kRexPrefixCompareValue = 0x40;
#endif

  /// Opcode for a nop instruction.
  static constexpr uint8_t kNopInstructionOpcode = 0x90;

  /// Tests if the specified byte could be a REX prefix or not.
  /// @param byte Byte to test.
  /// @return `true` if it is a REX prefix, `false` if not.
  static inline bool CouldBeRexPrefix(const uint8_t byte)
  {
#ifdef _WIN64
    return ((byte & kRexPrefixMask) == kRexPrefixCompareValue);
#else
    // REX prefixes do not exist in 32-bit mode.
    return false;
#endif
  }

  X86Instruction::X86Instruction(void)
      : decodedInstruction(),
        address(nullptr),
        valid(false),
        possibleRexPrefix(0),
        positionDependentMemoryReference()
  {}

  int X86Instruction::PositionDependentMemoryReference::OperandLocationFromInstruction(
      xed_decoded_inst_t* const decodedInstruction)
  {
    // First, look through all the operands and check for any that are relative branch
    // displacements. The target of a relative branch displacement is position-dependent.
    const unsigned int numOperands = xed_inst_noperands(xed_decoded_inst_inst(decodedInstruction));
    for (unsigned int i = 0; i < numOperands; ++i)
    {
      const xed_operand_enum_t operandName =
          xed_operand_name(xed_inst_operand(xed_decoded_inst_inst(decodedInstruction), i));
      switch (operandName)
      {
        case XED_OPERAND_RELBR:
          return kReferenceIsRelativeBranchDisplacement;

        default:
          break;
      }
    }

#ifdef _WIN64
    // Next, scan for any memory reference operands that use the instruction pointer register as a
    // base. Per Intel documentation, RIP-relative addressing is only supported in 64-bit code,
    // which means that RIP is the only possible base register.
    const unsigned int numMemoryOperands =
        xed_decoded_inst_number_of_memory_operands(decodedInstruction);
    for (unsigned int i = 0; i < numMemoryOperands; ++i)
    {
      const xed_reg_enum_t baseRegister = xed_decoded_inst_get_base_reg(decodedInstruction, i);
      switch (baseRegister)
      {
        case XED_REG_RIP:
          return i;

        default:
          break;
      }
    }
#endif

    return kReferenceDoesNotExist;
  }

  bool X86Instruction::CanWriteJumpInstruction(const void* const from, const void* const to)
  {
    const int64_t displacement = reinterpret_cast<int64_t>(to) -
        (reinterpret_cast<int64_t>(from) + static_cast<int64_t>(kJumpInstructionLengthBytes));
    return (displacement <= INT32_MAX && displacement >= INT32_MIN);
  }

  void X86Instruction::FillWithNop(void* const buf, const size_t numBytes)
  {
    uint8_t* const bufBytes = reinterpret_cast<uint8_t*>(buf);

    for (size_t i = 0; i < numBytes; ++i)
      bufBytes[i] = kNopInstructionOpcode;
  }

  bool X86Instruction::WriteJumpInstruction(
      void* const where, const int whereSizeBytes, const void* const to)
  {
    if (whereSizeBytes < kJumpInstructionLengthBytes) return false;

    if (false == CanWriteJumpInstruction(where, to)) return false;

    uint8_t* const whereBytes = reinterpret_cast<uint8_t*>(where);
    const int64_t displacement = reinterpret_cast<int64_t>(to) -
        (reinterpret_cast<int64_t>(where) + static_cast<int64_t>(kJumpInstructionLengthBytes));

    for (int i = 0; i < sizeof(kJumpInstructionPreamble); ++i)
      whereBytes[i] = kJumpInstructionPreamble[i];

    *reinterpret_cast<int32_t*>(&whereBytes[sizeof(kJumpInstructionPreamble)]) =
        static_cast<int32_t>(displacement);
    return true;
  }

  bool X86Instruction::CanSetMemoryDisplacementTo(const int64_t displacement) const
  {
    if (false == valid) return false;

    if (PositionDependentMemoryReference::kReferenceDoesNotExist ==
        positionDependentMemoryReference.GetOperandLocation())
      return false;

    return (
        displacement >= GetMinMemoryDisplacement() && displacement <= GetMaxMemoryDisplacement());
  }

  bool X86Instruction::DecodeInstruction(const void* instruction, const int maxLengthBytes)
  {
    xed_decoded_inst_zero_set_mode(&decodedInstruction, &kXedMachineState);

    if (XED_ERROR_NONE !=
        xed_decode(
            &decodedInstruction,
            reinterpret_cast<const unsigned char*>(instruction),
            maxLengthBytes))
    {
      address = nullptr;
      positionDependentMemoryReference.ClearOperandLocation();
      possibleRexPrefix = 0;
      valid = false;
      return false;
    }

    address = instruction;
    positionDependentMemoryReference.SetFromInstruction(&decodedInstruction);
    possibleRexPrefix =
        (CouldBeRexPrefix(reinterpret_cast<const uint8_t*>(instruction)[0])
             ? reinterpret_cast<const uint8_t*>(instruction)[0]
             : 0);
    valid = true;
    return true;
  }

  int X86Instruction::EncodeInstruction(void* const buf, const int maxLengthBytes) const
  {
    const unsigned int decodedLength = GetLengthBytes();

    if (false == valid || maxLengthBytes < 0 ||
        static_cast<unsigned int>(maxLengthBytes) < decodedLength)
      return 0;

    xed_encoder_request_t toEncode = decodedInstruction;
    xed_encoder_request_init_from_decode(&toEncode);

    unsigned int encodedLength = 0;
    if (XED_ERROR_NONE !=
        xed_encode(
            &toEncode, reinterpret_cast<unsigned char*>(buf), maxLengthBytes, &encodedLength))
      return 0;

    if (encodedLength > decodedLength)
    {
      // It should never be the case that the encoded instruction actually has a larger size than
      // the originally-decoded instruction./ The only changes that can be made to a decoded
      // instruction are changes to position-dependent memory offsets, which are size-preserving
      // operations.
      return 0;
    }
    else if (encodedLength < decodedLength)
    {
      // Sometimes the encoder does not preserve all the bytes of the original instruction if they
      // are deemed unnecessary because they would be ignored by the processor. To ensure there is
      // no length discrepancy between the original decoded instruction and the newly-encoded
      // enstruction, pad to the length of the original instruction. This is particularly important
      // in case the original instruction has a position-dependent memory reference, because
      // shortening its length can invalidate its displacement value.

      const unsigned int lengthDiscrepancy = decodedLength - encodedLength;

      for (unsigned int i = 0; i < encodedLength; ++i)
        reinterpret_cast<uint8_t*>(buf)[decodedLength - 1 - i] =
            reinterpret_cast<uint8_t*>(buf)[decodedLength - 1 - lengthDiscrepancy - i];

      FillWithNop(buf, lengthDiscrepancy);

#ifdef _WIN64
      // If the decoded instruction had a REX prefix and the newly-encoded instruction does not,
      // this means the encoder removed it because it has no functional purpose. However, REX
      // prefixes are used for other things besides just functional changes. For example, Windows
      // uses "rex.w jmp" to indicate a function epilogue. Therefore, in this case the REX prefix
      // should be reintroduced. REX prefixes only exist in 64-bit mode.
      if ((true == CouldBeRexPrefix(possibleRexPrefix)) &&
          (false == CouldBeRexPrefix(reinterpret_cast<uint8_t*>(buf)[lengthDiscrepancy])))
        reinterpret_cast<uint8_t*>(buf)[lengthDiscrepancy - 1] = possibleRexPrefix;
#endif

      encodedLength = decodedLength;
    }

    return static_cast<int>(encodedLength);
  }

  void* X86Instruction::GetAbsoluteMemoryReferenceTarget(void) const
  {
    const int64_t displacement = GetMemoryDisplacement();
    if (kInvalidMemoryDisplacement == displacement) return nullptr;

    const int64_t base = static_cast<int64_t>(reinterpret_cast<intptr_t>(address)) +
        static_cast<int64_t>(GetLengthBytes());
    return reinterpret_cast<void*>(static_cast<size_t>(base) + static_cast<size_t>(displacement));
  }

  int X86Instruction::GetLengthBytes(void) const
  {
    if (false == valid) return -1;

    return static_cast<int>(xed_decoded_inst_get_length(&decodedInstruction));
  }

  int64_t X86Instruction::GetMaxMemoryDisplacement(void) const
  {
    const int64_t memoryDisplacementWidthBits =
        static_cast<int64_t>(GetMemoryDisplacementWidthBits());
    if (0ll == memoryDisplacementWidthBits) return kInvalidMemoryDisplacement;

    return (1ll << (memoryDisplacementWidthBits - 1ll)) - 1ll;
  }

  int64_t X86Instruction::GetMemoryDisplacement(void) const
  {
    if (false == valid) return kInvalidMemoryDisplacement;

    switch (positionDependentMemoryReference.GetOperandLocation())
    {
      case PositionDependentMemoryReference::kReferenceDoesNotExist:
        return kInvalidMemoryDisplacement;

      case PositionDependentMemoryReference::kReferenceIsRelativeBranchDisplacement:
        return static_cast<int64_t>(xed_decoded_inst_get_branch_displacement(&decodedInstruction));

      default:
        return static_cast<int64_t>(xed_decoded_inst_get_memory_displacement(
            &decodedInstruction, positionDependentMemoryReference.GetOperandLocation()));
    }
  }

  int X86Instruction::GetMemoryDisplacementWidthBits(void) const
  {
    if (false == valid) return 0;

    switch (positionDependentMemoryReference.GetOperandLocation())
    {
      case PositionDependentMemoryReference::kReferenceDoesNotExist:
        return 0;

      case PositionDependentMemoryReference::kReferenceIsRelativeBranchDisplacement:
        return static_cast<int64_t>(
            xed_decoded_inst_get_branch_displacement_width_bits(&decodedInstruction));

      default:
        return static_cast<int64_t>(xed_decoded_inst_get_memory_displacement_width_bits(
            &decodedInstruction, positionDependentMemoryReference.GetOperandLocation()));
    }
  }

  int64_t X86Instruction::GetMinMemoryDisplacement(void) const
  {
    const int64_t memoryDisplacementWidthBits =
        static_cast<int64_t>(GetMemoryDisplacementWidthBits());
    if (0ll == memoryDisplacementWidthBits) return kInvalidMemoryDisplacement;

    return INT64_MIN >> (64ll - memoryDisplacementWidthBits);
  }

  bool X86Instruction::IsPadding(void) const
  {
    if (1 != GetLengthBytes()) return false;

    const uint8_t instructionBinary = *reinterpret_cast<const uint8_t*>(GetAddress());
    switch (instructionBinary)
    {
      case 0x90: // nop
      case 0xcc: // int 3
        return true;

      default:
        return false;
    }
  }

  bool X86Instruction::IsPaddingWithLengthAtLeast(int numBytes) const
  {
    static constexpr int kMinNumBytesForPaddingBuffer = 4;

    if (false == IsPadding()) return false;

    const int numBytesToCheck =
        ((numBytes < kMinNumBytesForPaddingBuffer) ? kMinNumBytesForPaddingBuffer : numBytes);
    const uint8_t* const paddingBuffer = reinterpret_cast<const uint8_t*>(GetAddress());
    const uint8_t expectedByte = paddingBuffer[0];

    for (int i = 1; i < numBytesToCheck; ++i)
    {
      if (paddingBuffer[i] != expectedByte) return false;
    }

    return true;
  }

  bool X86Instruction::IsTerminal(void) const
  {
    if (false == valid) return false;

    switch (xed_decoded_inst_get_category(&decodedInstruction))
    {
      case XED_CATEGORY_RET:
      case XED_CATEGORY_UNCOND_BR:
        return true;

      default:
        return false;
    }
  }

  bool X86Instruction::PrintDisassembly(wchar_t* const buf, const size_t numChars) const
  {
    if (false == valid) return false;

    Infra::TemporaryBuffer<char> narrowCharDisassembly;
    if (false ==
        xed_format_context(
            XED_SYNTAX_INTEL,
            &decodedInstruction,
            narrowCharDisassembly.Data(),
            narrowCharDisassembly.Capacity(),
            reinterpret_cast<xed_uint64_t>(address),
            nullptr,
            nullptr))
      return false;

    return (
        0 ==
        mbstowcs_s(
            nullptr,
            buf,
            numChars,
            narrowCharDisassembly.Data(),
            narrowCharDisassembly.Capacity()));
  }

  bool X86Instruction::SetMemoryDisplacement(const int64_t displacement)
  {
    if (false == CanSetMemoryDisplacementTo(displacement)) return false;

    switch (positionDependentMemoryReference.GetOperandLocation())
    {
      case PositionDependentMemoryReference::kReferenceDoesNotExist:
        return false;

      case PositionDependentMemoryReference::kReferenceIsRelativeBranchDisplacement:
        xed_decoded_inst_set_branch_displacement_bits(
            &decodedInstruction, static_cast<int>(displacement), GetMemoryDisplacementWidthBits());
        break;

      default:
        xed_decoded_inst_set_memory_displacement_bits(
            &decodedInstruction,
            static_cast<long long>(displacement),
            GetMemoryDisplacementWidthBits());
        break;
    }

    return (GetMemoryDisplacement() == displacement);
  }
} // namespace Hookshot

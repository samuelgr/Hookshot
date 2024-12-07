/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file Trampoline.cpp
 *   Implementation of some functionality for individual trampolines.
 **************************************************************************************************/

#include "Trampoline.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include <Infra/Core/ProcessInfo.h>
#include <Infra/Core/TemporaryBuffer.h>

#include "DependencyProtect.h"
#include "Message.h"
#include "X86Instruction.h"

namespace Hookshot
{
  /// Used in place of a disassembly string whenever a disassembly operation fails.
  static constexpr std::wstring_view kDisassemblyFailedString = L"(failed to disassemble)";

  /// Loaded into the hook region of the trampoline at initialization time.
  /// Provides the needed preamble and allows room for the hook function address to be specified
  /// after-the-fact. When the trampoline is set, the hook function address is filled in after the
  /// code contained here.
  static constexpr uint8_t kHookCodePreamble[] = {
#ifdef _WIN64
      // nop
      0x66,
      0x90,

      // jmp QWORD PTR [rip]
      0xff,
      0x25,
      0x00,
      0x00,
      0x00,
      0x00,
#else
      // nop
      0x66,
      0x0f,
      0x1f,
      0x84,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,

      // nop
      0x66,
      0x90,

      // jmp rel32
      0xe9,
#endif
  };

  // Used to verify that the amount of preamble code is just right.
  // If too much, a pointer to the hook function will not fit into the allowed space of the hook
  // code region. To fix, increase the size of the hook region of the trampoline or decrease the
  // size of the code. If too little, the code will not execute correctly because there will be a
  // gap between the code and the pointer. To fix, pad the hook code preamble with nop instructions.
  static_assert(
      !(sizeof(kHookCodePreamble) + sizeof(void*) > Trampoline::kTrampolineSizeHookFunctionBytes),
      "Hook code preamble is too big. Either increase the size of the hook region of the trampoline or decrease the size of the preamble code.");
  static_assert(
      !(sizeof(kHookCodePreamble) + sizeof(void*) < Trampoline::kTrampolineSizeHookFunctionBytes),
      "Hook code preamble is too small. Pad with nop instructions to increase the size.");

  /// Loaded into the trampoline at initialization time. This is the "int 3" instruction, which is
  /// often used as padding and acts as an "uninitialized poison" by causing the program to break
  /// into the debugger when executed.
  static constexpr uint8_t kTrampolineCodeDefault = 0xcc;

  Trampoline::Trampoline(void) : code()
  {
    Reset();
  }

  void Trampoline::Reset(void)
  {
    static_assert(
        sizeof(kHookCodePreamble[0]) == sizeof(code.hook.byte[0]),
        L"Hook code preamble element size mismatch.");
    static_assert(
        sizeof(kTrampolineCodeDefault) == sizeof(code.hook.byte[0]),
        L"Default instruction size mismatch.");

    for (int i = 0; i < _countof(kHookCodePreamble); ++i)
      code.hook.byte[i] = kHookCodePreamble[i];

    for (int i = _countof(kHookCodePreamble); i < _countof(code.hook.byte); ++i)
      code.hook.byte[i] = kTrampolineCodeDefault;

    for (int i = 0; i < _countof(code.original.byte); ++i)
      code.original.byte[i] = kTrampolineCodeDefault;
  }

  void Trampoline::SetHookFunction(const void* hookFunc)
  {
    Message::OutputFormatted(
        Message::ESeverity::Info,
        L"Trampoline at 0x%llx is being set up with hook function 0x%llx.",
        (long long)this,
        (long long)hookFunc);
    code.hook.ptr[_countof(code.hook.ptr) - 1] = ValueForHookAddress(hookFunc);
    Protected::Windows_FlushInstructionCache(
        Infra::ProcessInfo::GetCurrentProcessHandle(), &code.hook, sizeof(code.hook));
  }

  bool Trampoline::SetOriginalFunction(const void* originalFunc)
  {
    Message::OutputFormatted(
        Message::ESeverity::Info,
        L"Trampoline at 0x%llx is being set up with original function 0x%llx.",
        (long long)this,
        (long long)originalFunc);

    // Sanity check. Make sure the original function is not too far away from this trampoline.
    if (false == X86Instruction::CanWriteJumpInstruction(originalFunc, &code.hook))
    {
      Message::OutputFormatted(
          Message::ESeverity::Warning,
          L"Set hook failed for function %llx because it is too far from the trampoline.",
          (long long)originalFunc);
      return false;
    }

    // This operation requires transplanting code from the location of the original function into
    // the original function part of the trampoline. This is done in several sub-parts, and more
    // details will be provided while executing each sub-part.

    // First, read and decode instructions until either enough bytes worth of instructions are
    // decoded to hold an unconditional jump or a terminal instruction is hit. If the latter happens
    // strictly before the former, and no extra padding space can be found, then setting this hook
    // failed due to there not being enough bytes of function to transplant.

    // Second, iterate through all the decoded instructions and check for position-dependent memory
    // references. Update the displacements as needed for any such decoded instructions. If that is
    // not possible for even one instruction, then setting this hook failed.

    // Third, encode the decoded instructions into this trampoline's original function region. If
    // needed (i.e. the last of the decoded instructions is non-terminal), append an unconditional
    // jump instruction to the correct address within the original function. This will be completed
    // at the same time as the second sub-part.

    // First sub-part.
    const uint8_t* originalFunctionBytes = reinterpret_cast<const uint8_t*>(originalFunc);
    constexpr int numOriginalFunctionBytesNeeded = X86Instruction::kJumpInstructionLengthBytes;
    int numOriginalFunctionBytes = 0;
    int instructionIndex = 0;
    std::vector<X86Instruction> originalInstructions;

    Message::OutputFormatted(
        Message::ESeverity::Debug,
        L"Starting to decode instructions at 0x%llx, need %d bytes.",
        (long long)originalFunc,
        numOriginalFunctionBytesNeeded);

    while (numOriginalFunctionBytes < numOriginalFunctionBytesNeeded)
    {
      X86Instruction& decodedInstruction = originalInstructions.emplace_back();
      decodedInstruction.DecodeInstruction(&originalFunctionBytes[numOriginalFunctionBytes]);

      if (false == decodedInstruction.IsValid())
      {
        Message::OutputFormatted(
            Message::ESeverity::Debug, L"Instruction %d - Invalid instruction.", instructionIndex);
        return false;
      }

      if (Message::WillOutputMessageOfSeverity(Message::ESeverity::Debug))
      {
        Infra::TemporaryBuffer<wchar_t> disassembly;
        const bool disassemblyResult =
            decodedInstruction.PrintDisassembly(disassembly.Data(), disassembly.Capacity());
        Message::OutputFormatted(
            Message::ESeverity::Debug,
            L"Instruction %d - Decoded %d-byte instruction \"%s\"",
            instructionIndex,
            decodedInstruction.GetLengthBytes(),
            ((true == disassemblyResult) ? &disassembly[0] : kDisassemblyFailedString.data()));

        if (decodedInstruction.IsTerminal())
          Message::OutputFormatted(
              Message::ESeverity::Debug,
              L"Instruction %d - This is a terminal instruction.",
              instructionIndex);
      }

      numOriginalFunctionBytes += decodedInstruction.GetLengthBytes();
      instructionIndex += 1;

      if (decodedInstruction.IsTerminal()) break;
    }

    if (numOriginalFunctionBytes < numOriginalFunctionBytesNeeded)
    {
      // Unable to decode a sufficient number of bytes worth of original function instructions. The
      // terminal instruction might be followed by some padding bytes that were inserted for
      // alignment purposes. If so, Hookshot can overwrite some of them safely, which would allow
      // the hooking process to continue. It is not necessary to transplant padding instructions
      // because they are not intended to be executed.

      const int numBytesShort = numOriginalFunctionBytesNeeded - numOriginalFunctionBytes;

      X86Instruction hopefullyPaddingInstruction;
      hopefullyPaddingInstruction.DecodeInstruction(
          &originalFunctionBytes[numOriginalFunctionBytes]);

      if (hopefullyPaddingInstruction.IsPaddingWithLengthAtLeast(numBytesShort))
      {
        if (Message::WillOutputMessageOfSeverity(Message::ESeverity::Debug))
        {
          Infra::TemporaryBuffer<wchar_t> disassembly;
          const bool disassemblyResult = hopefullyPaddingInstruction.PrintDisassembly(
              disassembly.Data(), disassembly.Capacity());
          Message::OutputFormatted(
              Message::ESeverity::Debug,
              L"Decoded a total of %d byte(s), needed %d. This is insufficient, but at least %d byte(s) of padding instruction \"%s\" are available. Proceeding.",
              numOriginalFunctionBytes,
              numOriginalFunctionBytesNeeded,
              numBytesShort,
              ((true == disassemblyResult) ? &disassembly[0] : kDisassemblyFailedString.data()));
        }
      }
      else
      {
        Message::OutputFormatted(
            Message::ESeverity::Debug,
            L"Decoded a total of %d byte(s), needed %d. This is insufficient, and padding bytes could not be used. Bailing.",
            numOriginalFunctionBytes,
            numOriginalFunctionBytesNeeded);
        return false;
      }
    }
    else
    {
      Message::OutputFormatted(
          Message::ESeverity::Debug,
          L"Decoded a total of %d byte(s), needed %d. This is sufficient. Proceeding.",
          numOriginalFunctionBytes,
          numOriginalFunctionBytesNeeded);
    }

    // Second and third sub-parts.
    int numTrampolineBytesWritten = 0;
    int numExtraTrampolineBytesUsed = 0;

    for (int i = 0; i < static_cast<int>(originalInstructions.size()); ++i)
    {
      const int numTrampolineBytesLeft =
          sizeof(code.original) - numTrampolineBytesWritten - numExtraTrampolineBytesUsed;
      void* const nextTrampolineAddressToWrite = &code.original.byte[numTrampolineBytesWritten];

      // Second sub-part. Handle any position-dependent memory references.
      if (originalInstructions[i].HasPositionDependentMemoryReference())
      {
        const int64_t originalDisplacement = originalInstructions[i].GetMemoryDisplacement();
        Message::OutputFormatted(
            Message::ESeverity::Debug,
            L"Instruction %d - Has a position-dependent memory reference with displacement 0x%llx.",
            i,
            (long long)originalDisplacement);

        // If the displacement is so small that it refers to another instruction that is also being
        // transplanted, then there is no need to modify it. Note the need to check both forwards
        // (positive) and backwards (negative) displacement directions.
        const int64_t minForwardDisplacementNeedingModification = static_cast<int64_t>(
            numOriginalFunctionBytes -
            (numTrampolineBytesWritten + originalInstructions[i].GetLengthBytes()));
        const int64_t minBackwardDisplacementNotNeedingMofification = static_cast<int64_t>(
            -1 * (numTrampolineBytesWritten + originalInstructions[i].GetLengthBytes()));

        if (originalDisplacement >= minForwardDisplacementNeedingModification ||
            originalDisplacement < minBackwardDisplacementNotNeedingMofification)
        {
          // There are no changes to the length of the instruction, so the change to the
          // displacement value is just the difference in its new and original locations in memory.
          const intptr_t newDisplacementValue =
              (reinterpret_cast<intptr_t>(originalInstructions[i].GetAddress()) -
               reinterpret_cast<intptr_t>(nextTrampolineAddressToWrite)) +
              static_cast<intptr_t>(originalDisplacement);
          Message::OutputFormatted(
              Message::ESeverity::Debug,
              L"Instruction %d - Transplanting from 0x%llx to 0x%llx, absolute target is 0x%llx, new displacement is 0x%llx.",
              i,
              reinterpret_cast<long long>(originalInstructions[i].GetAddress()),
              reinterpret_cast<long long>(nextTrampolineAddressToWrite),
              reinterpret_cast<long long>(
                  originalInstructions[i].GetAbsoluteMemoryReferenceTarget()),
              static_cast<long long>(newDisplacementValue));

          // Try to replace the displacement in the original instruction. If this fails, perhaps
          // using a 32-bit unconditional jump as an assist will help, especially if the original
          // instruction uses an 8-bit or 16-bit relative displacement. However, an assist like this
          // is only possible if the original instruction has a relative branch displacement, not a
          // position-relative data access displacement. Note that the latter case, RIP-relative
          // addressing, is only supported in 64-bit mode.
          if (false ==
              originalInstructions[i].SetMemoryDisplacement(
                  static_cast<int64_t>(newDisplacementValue)))
          {
            if (true == originalInstructions[i].HasRelativeBranchDisplacement())
            {
              // The way a jump assist works is by allocating space at the end of the trampoline for
              // an unconditional jump instruction that targets the same address targetted by the
              // original instruction. Variable numExtraTrampolineBytesUsed stores the number of
              // such bytes already allocated at the end of the trampoline. Then, replace the
              // displacement in the original instruction with a displacement value that takes it to
              // the jump assist instruction. This solution works for any instruction that uses rel8
              // and rel16 branch displacements (such as loop, xbegin, etc.), not just conditional
              // and unconditional jumps.

              numExtraTrampolineBytesUsed += X86Instruction::kJumpInstructionLengthBytes;

              void* const jumpAssistAddress = reinterpret_cast<void*>(
                  reinterpret_cast<size_t>(&code.original.byte[_countof(code.original.byte)]) -
                  static_cast<size_t>(numExtraTrampolineBytesUsed));
              void* const jumpAssistTargetAddress =
                  originalInstructions[i].GetAbsoluteMemoryReferenceTarget();
              const intptr_t displacementValueToJumpAssist =
                  reinterpret_cast<intptr_t>(jumpAssistAddress) -
                  (reinterpret_cast<intptr_t>(nextTrampolineAddressToWrite) +
                   static_cast<intptr_t>(originalInstructions[i].GetLengthBytes()));

              Message::OutputFormatted(
                  Message::ESeverity::Debug,
                  L"Instruction %d - Failed to set new displacement, but will attempt to use a jump assist (from=0x%llx, to=0x%llx, disp=0x%llx, target=0x%llx) instead.",
                  i,
                  reinterpret_cast<long long>(nextTrampolineAddressToWrite),
                  reinterpret_cast<long long>(jumpAssistAddress),
                  static_cast<long long>(displacementValueToJumpAssist),
                  reinterpret_cast<long long>(jumpAssistTargetAddress));

              if (false ==
                  originalInstructions[i].SetMemoryDisplacement(
                      static_cast<int64_t>(displacementValueToJumpAssist)))
              {
                Message::OutputFormatted(
                    Message::ESeverity::Debug,
                    L"Instruction %d - Jump assist failed, unable to set original instruction displacement.",
                    i);
                return false;
              }

              if (false ==
                  X86Instruction::WriteJumpInstruction(
                      jumpAssistAddress,
                      X86Instruction::kJumpInstructionLengthBytes,
                      jumpAssistTargetAddress))
              {
                Message::OutputFormatted(
                    Message::ESeverity::Debug,
                    L"Instruction %d - Jump assist failed, unable write jump assist instruction.",
                    i);
                return false;
              }

              Message::OutputFormatted(
                  Message::ESeverity::Debug,
                  L"Instruction %d - Jump assist succeeded, encoded %d extra bytes at 0x%llx.",
                  i,
                  X86Instruction::kJumpInstructionLengthBytes,
                  (long long)jumpAssistAddress);
            }
            else
            {
              Message::OutputFormatted(
                  Message::ESeverity::Debug,
                  L"Instruction %d - Failed to set new displacement, and cannot use a jump assist.",
                  i);
              return false;
            }
          }
        }
        else
          Message::OutputFormatted(
              Message::ESeverity::Debug,
              L"Instruction %d - Displacement is short enough, no modification required.",
              i);
      }

      // Third sub-part. Re-encode the instruction.
      int numEncodedBytes = originalInstructions[i].EncodeInstruction(
          nextTrampolineAddressToWrite, numTrampolineBytesLeft);

      if (0 == numEncodedBytes)
      {
        Message::OutputFormatted(
            Message::ESeverity::Debug,
            L"Instruction %d - Failed to encode at 0x%llx.",
            i,
            (long long)nextTrampolineAddressToWrite);
        return false;
      }

      Message::OutputFormatted(
          Message::ESeverity::Debug,
          L"Instruction %d - Encoded %d byte(s) at 0x%llx.",
          i,
          numEncodedBytes,
          (long long)nextTrampolineAddressToWrite);
      numTrampolineBytesWritten += numEncodedBytes;
    }

    // If the last transplanted instruction is terminal, then the entirety of the original function
    // was transplanted, so there is no need to jump to an address in the original function.
    // Otherwise, there are more instructions left in the original function, so make sure to jump to
    // them after executing the trasnplanted instructions.
    if (false == originalInstructions.back().IsTerminal())
    {
      const int numTrampolineBytesLeft =
          sizeof(code.original) - numTrampolineBytesWritten - numExtraTrampolineBytesUsed;
      Message::OutputFormatted(
          Message::ESeverity::Debug,
          L"Final encoded instruction is non-terminal, so adding a jump to 0x%llx with %d byte(s) free in the trampoline.",
          (long long)&originalFunctionBytes[numOriginalFunctionBytes],
          numTrampolineBytesLeft);

      if (false ==
          X86Instruction::WriteJumpInstruction(
              &code.original.byte[numTrampolineBytesWritten],
              numTrampolineBytesLeft,
              &originalFunctionBytes[numOriginalFunctionBytes]))
      {
        Message::OutputFormatted(
            Message::ESeverity::Debug, L"Failed to write terminal jump instruction.");
        return false;
      }
    }

    Protected::Windows_FlushInstructionCache(
        Infra::ProcessInfo::GetCurrentProcessHandle(), &code.original, sizeof(code.original));
    return true;
  }
} // namespace Hookshot

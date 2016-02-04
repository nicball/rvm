Module Instruction
  ( OprandType (..)
  , Instr (..)
  ) where

import Data.Word

data OprandType
  = Uint8
  | Uint32
  | Pointer
  | Adt

data Instruction
  = Add OprandType
  | Sub OprandType
  | Mul OprandType
  | Div OprandType
  | Rem OprandType
  | And OprandType
  | Or OprandType
  | Xor OprandType
  | Not OprandType

  | Dup
  | Drop
  | Ldc Word32
  | Ldloc Word32
  | Stloc Word32
  | Ldarg Word32
  | Starg Word32
  | Call Word32
  | Ret
  | Ldloca Word32
  | Ldarga Word32
  | Ldfuna Word32
  | Calla
  | Ldind
  | Stind

  | Teq OprandType
  | Tne OprandType
  | Tlt OprandType
  | TltS OprandType
  | Tle OprandType
  | TleS OprandType
  | Tgt OprandType
  | TgtS OprandType
  | Tge OprandType
  | TgeS OprandType

  | Br Word32
  | BrTrue Word32
  | Switch [Word32]

  | Mkadt Word32 Word32
  | Dladt
  | Ldctor
  | Ldfld Word32
  | Stfld Word32
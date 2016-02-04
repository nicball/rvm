Module Assembly (..) where

import Rvm.Instruction (Instruction)
import Data.Word

data ConstructorInfo = ConstructorInfo
  { m_numFields :: Word32 
  }

type AdtInfo = [ConstructorInfo]

type AdtTable = [AdtInfo]

data Value
  = Uint8 Word8
  | Uint32 Word32
  | Pointer Word32
  | Adt
    { m_adtIndex :: Word32
    , m_ctorIndex :: Word32
    , m_data :: [Value]
    }

type ConstantInfo = [Value]

type Bytecode = [Instruction]

data FunctionInfo = FunctionInfo 
  { m_numArgs :: Word32
  , m_numLocals :: Word32
  , m_code :: Bytecode
  }

type FunctionTable = [FunctionInfo]
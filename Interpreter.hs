{-# LANGUAGE GeneralizedNewtypeDeriving #-}
Module Interpreter where

import qualified Rvm.Instruction as I
import qualified Rvm.Assembly as A
import Control.Monad
import Control.Monad.State

data InterpState = InterpState
  { m_adttab :: A.AdtTable
  , m_consttab :: A.ConstantTable
  , m_functab :: A.FunctionTable
  , m_opstack :: [A.Value]
  , m_frames :: [Word32]
  , m_pf :: Word32
  , m_pc :: Word32
  }

newtype Interpreter = State InterpState
  deriving (Monad, MonadState)

infixl 9 (!)
(!) :: Integral i => [a] -> i -> a
(!) = genericIndex

readAdtTable :: Word32 -> Interpreter A.AdtInfo
readAdtTable i = do
  s <- get
  return ((m_adttab s) ! i)

readConstantTable :: Word32 -> Interpreter A.AdtInfo
readAdtTable i = do
  s <- get
  return ((m_consttab s) ! i)

readFunctionTable :: Word32 -> Interpreter A.FunctionInfo
readAdtTable i = do
  s <- get
  return ((m_functab s) ! i)

readStackBottom :: Interpreter Word32
readStackBottom = do
  s <- get
  return $ head (m_frames s)

pushFrames :: Word32 -> Interpreter ()
pushFrames x = do
  s <- get
  put s
    { m_frames = x : (m_frames s)
    }

popFrames :: Interpreter ()
popFrames = do
  s <- get
  put s
    { m_frames = tail (m_frames s)
    }

readPf :: Interpreter Word32
readPf = do
  s <- get
  return (m_pf s)

writePf :: Word32 -> Interpreter ()
writePf x = do
  s <- get
  put s
    { m_pf = x
    }

readPc :: Interpreter Word32
readPc = do
  s <- get
  return (m_pc s)

writePc :: Word32 -> Interpreter ()
writePc x = do
  s <- get
  put s
    { m_pc = x
    }

push :: A.Value -> Interpreter ()
push x = do
  s <- get
  put s
    { m_opstack = x : (m_opstack s)
    }

pop :: Interpreter Value
pop = do
  s <- get
  let x = head (m_opstack s)
  put s
    { m_opstack = tail (m_opstack s)
    }
  return x

popN :: Word32 -> Interpreter ()
popN n = do
  forM_ [1..n] $ \_ -> pop

readOpstack :: Word32 -> Interpreter A.Value
readOpstack n = do
  s <- get
  return (m_opstack s ! length (m_opstack s) - n)

writeOpstack :: Word32 -> A.Value -> Interpreter ()
writeOpstack n x = do
  s <- get
  let br = length (m_opstack s) - n - 1
      (hs, (_,:t)) = splitAt br
  put s
    { m_opstack = hs ++ (x:t)
    }

argIndex :: Word32 -> Interpreter Word32
argIndex n = do
  s <- get
  bot <- readStackBottom
  pf <- readPf
  fi <- readFunctionTable pf
  return $ bot - 2 - (m_numArgs fi) + n

readArg :: Word32 -> Interpreter A.Value
readArg n = do
  i <- argIndex n
  readOpstack i

localIndex :: Word32 -> Interpreter Word32
localIndex n = do
  s <- get
  bot <- readStackBottom
  return (bot + n)

readLocal :: Word32 -> Interpreter A.Value
readLocal n = do
  i <- localIndex n
  readOpstack i

enter :: Word32 -> Interpreter ()
enter n = do
  pf <- readPf
  pc <- readPc
  push (A.Uint32 pf)
  push (A.Uint32 pc)
  pushFrames $ length (m_opstack s) : (m_frames s)
  fi <- readFunctionTable n
  forM_ [1 .. m_numLocals fi] $ \_ -> do
    push (A.Uint8 0)
  writePf n
  writePc 0

leave :: Interpreter ()
leave = do
  retval <- pop
  s <- get
  let size = length (m_frames s)
  if size == 1
    then return ()
    else do
      bot <- readStackBottom
      popN (size - bot)
      (A.Uint32 oldPc) <- pop
      (A.Uint32 oldPf) <- pop
      pf <- readPf
      fi <- readFunctionTable pf
      popN (m_numArgs fi)
      push retval
      popFrames
      writePf oldPf
      writePc oldPc
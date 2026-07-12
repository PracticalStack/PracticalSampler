NeuralNetwork::process(var input) -> var

Thread safety: WARNING -- uses ScopedTryReadLock (non-blocking, skips if write lock held). No allocations on the processing path. Inference computation is O(model_size) per call.
Runs inference on the loaded model. Accepts a single float, an array of floats, or a
Buffer as input. Returns a single float when the model has one output, or a Buffer
reference when it has multiple outputs. If an output cable is connected, the first output
value is automatically sent to it. Silently returns zero if a model swap is in progress.
Required setup:
  const var nn = Engine.createNeuralNetwork("MyNetwork");
  nn.loadPytorchModel(modelJSON);
Dispatch/mechanics:
  ScopedTryReadLock -> if locked, return 0.0
    -> single input: ModelBase::process(&input, &output)
    -> array input: copy to inputBuffer, process, return outputBuffer or float
    -> buffer input: use buffer pointer directly
    -> if output cable connected, send first output value
Anti-patterns:
  - [BUG] Array/Buffer input size validation uses isPositiveAndBelow(expectedSize, inputSize)
    which requires inputSize > expectedSize (strict greater-than). An array with exactly
    the model's input count fails silently -- must have at least one extra element.
  - Do NOT call process per-sample in script callbacks for audio-rate use -- use the
    math.neural scriptnode node instead. Script callbacks cannot sustain sample-rate
    processing.
  - Returns 0.0 silently when a write lock is held during model swap -- no error or
    indication that processing was skipped
Source:
  ScriptingApiObjects.cpp  ScriptNeuralNetwork::process()
    -> ScopedTryReadLock on nn->lock
    -> ModelBase::process(inputPtr, outputPtr)
    -> outputCable->sendValue(output[0]) if connected

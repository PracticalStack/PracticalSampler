NeuralNetwork::createModelJSONFromTextFile(ScriptObject fileObject) -> JSON

Thread safety: UNSAFE -- file I/O (reads file content from disk)
Parses a Pytorch model text file (output of Python's print(model)) and returns a
JSON array describing the network layers. The returned JSON can be passed directly
to build. Supports Sequential containers with Linear, Tanh, ReLU, and Sigmoid layers.
Required setup:
  const var nn = Engine.createNeuralNetwork("MyNetwork");
  const var modelFile = FileSystem.getFolder(FileSystem.Scripts).getChildFile("model.txt");
Dispatch/mechanics:
  ScriptFile -> loadFileAsString() -> PytorchParser::createJSONModel(text)
    -> parses Sequential/Linear/Tanh/ReLU/Sigmoid patterns
    -> returns JSON array of {type, name, inputs, outputs, isActivation}
Pair with:
  build -- pass the returned JSON to build the network topology
  loadWeights -- load trained parameters after building
Anti-patterns:
  - [BUG] Returns an empty object silently if the argument is not a valid ScriptFile.
    No error message is shown
Source:
  ScriptingApiObjects.cpp  ScriptNeuralNetwork::createModelJSONFromTextFile()
    -> PytorchParser::createJSONModel(fileContent)

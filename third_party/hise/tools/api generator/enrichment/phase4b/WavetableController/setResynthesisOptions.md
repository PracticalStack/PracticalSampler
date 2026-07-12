WavetableController::setResynthesisOptions(JSON optionData) -> undefined

Thread safety: UNSAFE -- parses JSON object, constructs ResynthesisOptions struct with string operations and heap allocations
Sets resynthesis options from a JSON object. Unspecified properties retain compiled
defaults. Also attaches this controller as the error handler for the resynthesis
process. Options are applied with dontSendNotification -- no automatic resynthesis.

Pair with:
  getResynthesisOptions -- retrieve current options, modify, pass back
  resynthesise -- must call explicitly after setting options

Anti-patterns:
  - Do NOT omit RemoveNoise from the options object -- defaults to the value of
    ReverseOrder instead of true due to a deserialization bug. Always set explicitly.
  - Do NOT expect automatic resynthesis after calling -- options are applied with
    dontSendNotification, call resynthesise() explicitly

Source:
  ScriptingApiObjects.cpp:5227  setResynthesisOptions()
    -> ResynthesisOptions::fromVar(optionData)
    -> options.errorHandler = this (connects WeakErrorHandler)
    -> wt->setResynthesisOptions(options, dontSendNotification)
  WavetableTools.cpp:41  fromVar() deserializes JSON to ResynthesisOptions struct

BeatportManager (object)
Obtain via: Engine.createBeatportManager()

Beatport SDK DRM integration for product access validation and licensing.
Wraps the Beatport authentication system behind the HISE_INCLUDE_BEATPORT
preprocessor flag (default: off). When disabled, operates in simulation mode
using a local JSON file for development and testing.

Common mistakes:
  - Calling validate() without creating validate_response.json in simulation
    mode -- throws a script error. Create {project}/AdditionalSourceCode/
    beatport/validate_response.json before calling validate().

Example:
  const bp = Engine.createBeatportManager();
  bp.setProductId("my-product-id");

  if (bp.isBeatportAccess())
  {
      var result = bp.validate();
      Console.print(trace(result));
  }

Methods (3):
  isBeatportAccess  setProductId  validate

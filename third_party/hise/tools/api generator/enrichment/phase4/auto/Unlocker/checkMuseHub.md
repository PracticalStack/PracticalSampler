Initiates an asynchronous MuseHub licence check. The callback receives a single boolean indicating whether the check succeeded. Requires `HISE_INCLUDE_MUSEHUB` to be enabled for real SDK integration in frontend builds.

```js
inline function onMuseHubResult(ok)
{
    if (ok)
        Console.print("MuseHub licence verified");
    else
        Console.print("MuseHub check failed");
}

ul.checkMuseHub(onMuseHubResult);
```

> [!Warning:Backend results are simulated] In the HISE IDE, the result is random (50% chance of success) with a fixed 2-second delay. Do not rely on backend results to test MuseHub integration logic.
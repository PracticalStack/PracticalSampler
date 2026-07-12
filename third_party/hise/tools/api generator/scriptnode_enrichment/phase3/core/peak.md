---
keywords: peak
summary:  Sends the maximum input value as control signal
author:   Christoph Hart
modified: 06.08.2021
---
  
This node will analyse the signal input and detect the (absolute) maximum value which is then sent as control value to any connected target. The value will be calculated using this formula:

```
output = Math.max(Math.abs(signalMin), Math.abs(signalMax))
```

So it will fold a negative signal back into the 0...1 range and send this as normalised value to its targets. If you want to convert a audio signal into this value range, use the [sig2mod](/scriptnode/list/math/sig2mod) node which will perform this conversion for you.

This makes it suitable for all kinds of modulation:

- envelope followers
- LFOs

However if you need a **raw output** of the signal without any processing, take a look at the [core.peak_unscaled](/scriptnode/list/core/peak_unscaled) node.

### Modulation Frequency

Be aware that this node will only send a single value after each audio buffer that it has processed. So if you need a fixed periodic update, you will ensure a fixed size processing using either the [`container.fix8_block`](/scriptnode/list/container/fix8_block) or - if you require a sample-accurate control signal - the [`container.frame2_block`](/scriptnode/list/container/frame2_block)

### Display Buffer

This node can also act as source for displaying a graph on your UI. It reads the values into a ring buffer that can be converted to a UI path using the [DisplayBuffer](/scripting/scripting-api/displaybuffer) API.

It supports the generic API for controlling the properties of the ring buffer, but is limited to a single property called `BufferLength` which must be any value between 512 and 65536:

```
{
  "BufferLength": 65536,
  "NumChannels": 1 // must always be 1 or the world will burn to the ground
}
```

> Note that you can click on the edit button at the bottom right and choose **Show in big popup** to see a resizable popup with the graph for better inspection of the values.
Rebinds this component to another slider-pack data source, another compatible complex-data component, or `-1` to return to its internal data object.

Use this to let one UI surface edit different datasets without rebuilding the component.

> [!Warning:Shared data updates both packs] If multiple packs share one data handle, writes in either view update both immediately. Guard callback logic to avoid feedback loops.

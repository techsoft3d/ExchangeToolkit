# Example: `normalize_coords`
This example loaded the specified input file and determines the global origin. If this origin is greater than a threshold value, it attempts to "normalize" the coordinates of the tessellated data contained within.

It does this by examining the path of each representation item instance. If any entity in the path contains a major cooredinate system offset, it makes an adjustment to that entity to normalize the instance.

The code is careful not to duplicate normalizations, considering part instancing is possible.

If there is no node in the instance path with a large translation, the tessellation itself is normalized.

The offset for normalization is stored as a json string attribute named `origin_offset` attached to the A3DAsmModelFile. It takes the following form:

```javascript
{
	offset : {
		x : 8.39693e+08,
		y : 8.19699e+08, 
		z : 23780.4
	}
}
```

Finally, the modified input file is written to the specified output file.

## Sample Usage
`normalize_coords PODIUM.ifc PODIUM.ifc_n.prc`

## Sample Output
```
Input file: PODIUM.ifc
Output file: PODIUM.ifc_n.prc
Exchange: /opt/local/ts3d/HOOPS_Exchange_2019_SP2_U2
Translation threshold is: 100000
Adjusting origin to: (8.39693e+08, 8.19699e+08, 23780.4)
Processing 6654 representation items.
Representation item with 1 instance.
- Translated instance path.
Representation item with 40 instances.
- Instance path already translated.
- Instance path already translated.
- Instance path already translated.
- Instance path already translated.
- Instance path already translated.
```
*snip!*
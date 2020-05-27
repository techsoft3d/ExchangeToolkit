# Example: bom
This example illustrates how to obtain a Bill of Materials from an arbitrary input CAD file. It is implemented to read the specified file, and write to standard output.

This sample illustrates the usefulness of the function `ts3d::getUniqueLeafEntities` in a clear and straightforward way.

## Sample Usage
`bom "_micro engine.CATProduct"`

## Sample Output
```
Input file: _micro engine.CATProduct
Exchange: /opt/local/ts3d/HOOPS_Exchange_2019_SP2_U2
"AXE": (1 instance)
"BEARING PR UP": (1 instance)
"HOUSING": (1 instance)
"BEARING PR DW": (1 instance)
"CYLINDER LINER": (1 instance)
"HOUSING TOP": (1 instance)
"CARBURETOR": (1 instance)
"SCREW BACK": (4 instances)
"SCREW TOP": (4 instances)
"CRANKSHAFT": (1 instance)
"HOUSING FRONT": (1 instance)
"BEARING CS": (1 instance)
"HOUSING BACK": (1 instance)
"PUSH ROD": (1 instance)
"PISTON": (1 instance)
```
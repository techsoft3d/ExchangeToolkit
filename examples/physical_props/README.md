# Example: `physical_props`
This example loads the specified input file and iterates over all instances of part definitions. It attempts to obtain density information from the part definition, then moves on to compute and display physical property information for each of the B-Rep models.

Using the instance path information, and the net transformation, units and B-Rep scaling, the physical properties are displayed in global coordinates.


## Sample Usage
`physical_props --density=7800. "_micro engine.CATProduct"`

## Sample Output
```
Default density (kg/m^3): 7800
Input file: /opt/local/ts3d/HOOPS_Exchange_Publish_2020_U1/samples/data/catiaV5/CV5_Micro_engine/_micro engine.CATProduct
Exchange: /opt/local/ts3d/HOOPS_Exchange_2019_SP2_U2
Unit (mm/unit): 1
Processing A3DAsmPartDefinition named "HOUSING TOP" (1 instance)
Part density (kg/m^3): 1000 (from part)
A3DRiBrepModel "MechanicalTool.1" [
	{
		Volume (mm^3): 14796.5
		Surface area (mm^2): 13370.6
		COG (mm): 10.661, -4.17272e-07, -3.47257e-07
		Mass (kg): 0.0147965
		Local moments of inertia (kg mm^2):
		     3.89091 -4.28656e-09  8.11206e-09
		-4.28656e-09      2.39606  3.64769e-07
		 8.11206e-09  3.64769e-07      2.37968
	}, 
]
Processing A3DAsmPartDefinition named "HOUSING BACK" (1 instance)
Part density (kg/m^3): 1000 (from part)
A3DRiBrepModel "MechanicalTool.1" [
	{
		Volume (mm^3): 10477.5
		Surface area (mm^2): 4595.52
		COG (mm): 6.01309, 28.5, -20
		Mass (kg): 0.0104775
		Local moments of inertia (kg mm^2):
		     2.35247  1.81214e-11 -3.80735e-11
		 1.81214e-11      1.28306  1.42046e-07
		-3.80735e-11  1.42046e-07      1.28306
	}, 
]
Processing A3DAsmPartDefinition named "HOUSING" (1 instance)
Part density (kg/m^3): 1000 (from part)
A3DRiBrepModel "MechanicalTool.1" [
	{
		Volume (mm^3): 22505.2
		Surface area (mm^2): 21161
		COG (mm): -0.00396946, 28.5134, -38.2547
		Mass (kg): 0.0225052
		Local moments of inertia (kg mm^2):
		    16.5568 4.39273e-05 -0.00134658
		4.39273e-05      14.084   -0.220729
		-0.00134658   -0.220729     5.99497
	}, 
]
Processing A3DAsmPartDefinition named "PUSH ROD" (1 instance)
Part density (kg/m^3): 1000 (from part)
A3DRiBrepModel "MechanicalTool.1" [
	{
		Volume (mm^3): 648.78
		Surface area (mm^2): 913.412
		COG (mm): -2.00953e-07, -5.03461e-07, -11.7624
		Mass (kg): 0.00064878
		Local moments of inertia (kg mm^2):
		     0.11604 -3.50401e-10 -6.92776e-10
		-3.50401e-10     0.111553 -6.79396e-09
		-6.92776e-10 -6.79396e-09   0.00645804
	}, 
]
Processing A3DAsmPartDefinition named "CYLINDER LINER" (1 instance)
Part density (kg/m^3): 1000 (from part)
A3DRiBrepModel "MechanicalTool.1" [
	{
		Volume (mm^3): 2157.65
		Surface area (mm^2): 4517.16
		COG (mm): -4.20713e-07, -18.3633, 0.0352238
		Mass (kg): 0.00215765
		Local moments of inertia (kg mm^2):
		    0.357454  4.44941e-10 -1.37432e-08
		 4.44941e-10     0.196609  -0.00626594
		-1.37432e-08  -0.00626594     0.364188
	}, 
]
Processing A3DAsmPartDefinition named "BEARING PR DW" (1 instance)
Part density (kg/m^3): 1000 (from part)
A3DRiBrepModel "MechanicalTool.1" [
	{
		Volume (mm^3): 109.956
		Surface area (mm^2): 263.894
		COG (mm): 1.35176e-15, 1.24914e-12, 2.35089e-16
		Mass (kg): 0.000109956
		Local moments of inertia (kg mm^2):
		0.000916298 4.18983e-20 6.70457e-11
		4.18983e-20  0.00137445 4.46839e-21
		6.70457e-11 4.46839e-21 0.000916298
	}, 
]
Processing A3DAsmPartDefinition named "BEARING PR UP" (1 instance)
Part density (kg/m^3): 1000 (from part)
A3DRiBrepModel "MechanicalTool.1" [
	{
		Volume (mm^3): 61.8501
		Surface area (mm^2): 189.674
		COG (mm): -1.56726e-16, 1.24912e-12, 2.50762e-15
		Mass (kg): 6.18501e-05
		Local moments of inertia (kg mm^2):
		0.000346296 1.37767e-20 2.12137e-11
		1.37767e-20 0.000434884 4.05557e-21
		2.12137e-11 4.05557e-21 0.000346296
	}, 
]
Processing A3DAsmPartDefinition named "CRANKSHAFT" (1 instance)
Part density (kg/m^3): 1000 (from part)
A3DRiBrepModel "MechanicalTool.1" [
	{
		Volume (mm^3): 8400.81
		Surface area (mm^2): 4913.31
		COG (mm): 0.108483, -37.1754, 3.41261e-08
		Mass (kg): 0.00840081
		Local moments of inertia (kg mm^2):
		     3.02455    0.0269304 -1.81272e-08
		   0.0269304     0.416512  3.23743e-09
		-1.81272e-08  3.23743e-09      3.03852
	}, 
]
Processing A3DAsmPartDefinition named "PISTON" (1 instance)
Part density (kg/m^3): 1000 (from part)
A3DRiBrepModel "MechanicalTool.1" [
	{
		Volume (mm^3): 2280.22
		Surface area (mm^2): 1506.53
		COG (mm): -1.62183e-08, 4.86387, 1.65904e-09
		Mass (kg): 0.00228022
		Local moments of inertia (kg mm^2):
		   0.0749436 -2.19219e-10  -1.1836e-08
		-2.19219e-10     0.101092  4.55933e-12
		 -1.1836e-08  4.55933e-12    0.0807744
	}, 
]
Processing A3DAsmPartDefinition named "HOUSING FRONT" (1 instance)
Part density (kg/m^3): 1000 (from part)
A3DRiBrepModel "MechanicalTool.1" [
	{
		Volume (mm^3): 19648.7
		Surface area (mm^2): 8366.91
		COG (mm): 12.5753, -0.496392, -1.94018e-07
		Mass (kg): 0.0196487
		Local moments of inertia (kg mm^2):
		    3.85646  -0.0400879 7.70593e-08
		 -0.0400879      3.7535 1.58094e-07
		7.70593e-08 1.58094e-07     3.60848
	}, 
]
Processing A3DAsmPartDefinition named "SCREW BACK" (4 instances)
Part density (kg/m^3): 1000 (from part)
A3DRiBrepModel "MechanicalTool.1" [
	{
		Volume (mm^3): 147.831
		Surface area (mm^2): 316.475
		COG (mm): 12.461, 5.75824e-06, 1.94842e-11
		Mass (kg): 0.000147831
		Local moments of inertia (kg mm^2):
		 0.000148208  1.25233e-08 -1.25707e-13
		 1.25233e-08    0.0219021 -7.39899e-12
		-1.25707e-13 -7.39899e-12    0.0219021
	}, 
	{
		Volume (mm^3): 147.831
		Surface area (mm^2): 316.475
		COG (mm): 12.461, 5.75824e-06, 1.94842e-11
		Mass (kg): 0.000147831
		Local moments of inertia (kg mm^2):
		 0.000148208  1.25233e-08 -1.25707e-13
		 1.25233e-08    0.0219021 -7.39899e-12
		-1.25707e-13 -7.39899e-12    0.0219021
	}, 
	{
		Volume (mm^3): 147.831
		Surface area (mm^2): 316.475
		COG (mm): 12.461, 5.75824e-06, 1.94842e-11
		Mass (kg): 0.000147831
		Local moments of inertia (kg mm^2):
		 0.000148208  1.25233e-08 -1.25707e-13
		 1.25233e-08    0.0219021 -7.39899e-12
		-1.25707e-13 -7.39899e-12    0.0219021
	}, 
	{
		Volume (mm^3): 147.831
		Surface area (mm^2): 316.475
		COG (mm): 12.461, 5.75824e-06, 1.94842e-11
		Mass (kg): 0.000147831
		Local moments of inertia (kg mm^2):
		 0.000148208  1.25233e-08 -1.25707e-13
		 1.25233e-08    0.0219021 -7.39899e-12
		-1.25707e-13 -7.39899e-12    0.0219021
	}, 
]
Processing A3DAsmPartDefinition named "SCREW TOP" (4 instances)
Part density (kg/m^3): 1000 (from part)
A3DRiBrepModel "MechanicalTool.1" [
	{
		Volume (mm^3): 84.3575
		Surface area (mm^2): 175.913
		COG (mm): 2.88092, 1.1053e-05, 1.99947e-11
		Mass (kg): 8.43575e-05
		Local moments of inertia (kg mm^2):
		 0.000119961  4.55111e-09 -3.13243e-14
		 4.55111e-09   0.00161535 -6.14183e-12
		-3.13243e-14 -6.14183e-12   0.00161535
	}, 
	{
		Volume (mm^3): 84.3575
		Surface area (mm^2): 175.913
		COG (mm): 2.88092, 1.1053e-05, 1.99947e-11
		Mass (kg): 8.43575e-05
		Local moments of inertia (kg mm^2):
		 0.000119961  4.55111e-09 -3.13243e-14
		 4.55111e-09   0.00161535 -6.14183e-12
		-3.13243e-14 -6.14183e-12   0.00161535
	}, 
	{
		Volume (mm^3): 84.3575
		Surface area (mm^2): 175.913
		COG (mm): 2.88092, 1.1053e-05, 1.99947e-11
		Mass (kg): 8.43575e-05
		Local moments of inertia (kg mm^2):
		 0.000119961  4.55111e-09 -3.13243e-14
		 4.55111e-09   0.00161535 -6.14183e-12
		-3.13243e-14 -6.14183e-12   0.00161535
	}, 
	{
		Volume (mm^3): 84.3575
		Surface area (mm^2): 175.913
		COG (mm): 2.88092, 1.1053e-05, 1.99947e-11
		Mass (kg): 8.43575e-05
		Local moments of inertia (kg mm^2):
		 0.000119961  4.55111e-09 -3.13243e-14
		 4.55111e-09   0.00161535 -6.14183e-12
		-3.13243e-14 -6.14183e-12   0.00161535
	}, 
]
Processing A3DAsmPartDefinition named "CARBURETOR" (1 instance)
Part density (kg/m^3): 1000 (from part)
A3DRiBrepModel "MechanicalTool.1" [
	{
		Volume (mm^3): 2417.44
		Surface area (mm^2): 2314.24
		COG (mm): 0.00189877, 0.269171, 1.93637
		Mass (kg): 0.00241744
		Local moments of inertia (kg mm^2):
		   0.222124 1.23686e-06 3.06476e-05
		1.23686e-06    0.108403  0.00228528
		3.06476e-05  0.00228528    0.146474
	}, 
]
Processing A3DAsmPartDefinition named "BEARING CS" (1 instance)
Part density (kg/m^3): 1000 (from part)
A3DRiBrepModel "MechanicalTool.1" [
	{
		Volume (mm^3): 2051.24
		Surface area (mm^2): 4217.73
		COG (mm): -3.77526e-08, -22.0386, 0.202625
		Mass (kg): 0.00205124
		Local moments of inertia (kg mm^2):
		    0.398593  5.03561e-10 -4.48564e-09
		 5.03561e-10     0.115809    0.0054703
		-4.48564e-09    0.0054703     0.401405
	}, 
]
Processing A3DAsmPartDefinition named "AXE" (1 instance)
Part density (kg/m^3): 1000 (from part)
A3DRiBrepModel "MechanicalTool.1" [
	{
		Volume (mm^3): 216.418
		Surface area (mm^2): 369.255
		COG (mm): 6.20202e-08, -8.5, 1.99142e-08
		Mass (kg): 0.000216418
		Local moments of inertia (kg mm^2):
		  0.00551378  1.07118e-12 -2.11529e-12
		 1.07118e-12   0.00065506  3.03971e-13
		-2.11529e-12  3.03971e-13   0.00551378
	} 
]
```

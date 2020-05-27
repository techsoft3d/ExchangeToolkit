# Example: `compare_brep_tess`
This example illustrates how to easily traverse both B-Rep and Tesselated faces, and it shows the relationships between the two.

First, the tessellation is obtained. A comparisons are made to ensure the number of B-Rep faces matches the number of tessellated faces. Similarly, comparisons between the loop counts and edge counts are made.

Finally, points from the tessellation are projected onto the B-Rep geometry and an error is computed. The error is compared to some threshold value that ensures some level of consistency between the two.


## Sample Usage
`compare_brep_tess "_micro engine.CATProduct"`

## Output
Really, there is no output aside from error conditions. The usefulness of this example lies entirely in code.
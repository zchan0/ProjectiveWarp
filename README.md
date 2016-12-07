# CPSC 6040 Homework 6 

> Get full specification on https://people.cs.clemson.edu/~dhouse/courses/404/hw/hw6/hw6.html

## Program function

Warp image with projective warp & bilinear warp, can do serveral transforms, including rotate, scale, translate, flip, shear and perspective.

## Usage

### Step 1

input command line:

```
warper [-b] inimage.png [outimage.png]
```

### Step 2

input transform specifications, MUST use following formats:

```
r theta      counter clockwise rotation about image origin, theta in degrees

s sx sy      scale (watch out for scale by 0!)

t dx dy      translate

f xf yf      flip - if xf = 1 flip horizontal, yf = 1 flip vertical

h hx hy      shear

p px py      perspective
```

### Done

input 'd' to see the result.

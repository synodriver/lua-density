# lua-density
```lua
local density = require("density")

local origin = "12345"

for i=1,8 do
    origin = origin..origin
end
print("origin len".. #origin)

local compressed = density.compress(origin, density.DENSITY_ALGORITHM_LION)
print("compressed len"..#compressed)

if (density.decompress(compressed, 100000) == origin) then
    print("pass")
else
    print("fail")
end
```
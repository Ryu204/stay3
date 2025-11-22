local bird = {
	name = "bird",
}
bird.__index = bird
setmetatable(bird, component)

return bird

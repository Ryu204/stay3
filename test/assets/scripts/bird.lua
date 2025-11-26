local bird = {
	name = "bird",
}
bird.__index = bird
setmetatable(bird, component)

function bird:start()
	self.elapsed_time = 0
	self.count = 0
end

function bird:update(dt)
	self.elapsed_time = self.elapsed_time + dt
	if self.elapsed_time > 1 then
		self.elapsed_time = self.elapsed_time - 1
		self.count = self.count + 1
		print(self.count)
	end
end

function bird:on_destroy()
	print("Bird goodbye :')")
end

return bird

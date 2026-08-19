--TweenService Wrapper
--Andrew Bereza
return function (object, properties, value, duration, style, direction)
	style = style or Enum.EasingStyle.Quad
	direction = direction or Enum.EasingDirection.Out

	duration = duration or 0.5

	local propertyGoals = {}

	local isTable = type(value) == "table"

	for i,property in pairs(properties) do
		propertyGoals[property] = isTable and value[i] or value
	end
	local tweenInfo = TweenInfo.new(
		duration,
		style,
		direction
	)
	local tween = game:GetService("TweenService"):Create(object,tweenInfo,propertyGoals)
	tween:Play()

	return tween
end
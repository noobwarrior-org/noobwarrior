extends ColorRect

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	self.visible = true
	var tween = create_tween()
	tween.tween_property(self, "color", Color(0, 0 ,0, 0), 3.0)
	tween.play()

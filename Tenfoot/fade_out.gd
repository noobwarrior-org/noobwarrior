extends ColorRect

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	self.visible = true
	await get_tree().create_timer(2.0).timeout
	var tween = create_tween()
	tween.tween_property(self, "color", Color(0, 0, 0, 0), 3.0)
	tween.play()

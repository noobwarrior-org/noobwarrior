extends Control

func fade_children(node : Node) -> void:
	for child in node.get_children():
		if child is Control:
			child.modulate.a = 0.0
			var tween = create_tween()
			tween.tween_property(child, "modulate:a", 1.0, 0.5)
			
			if child.get_child_count() > 0:
				fade_children(node)

func ShowMenu(path : String) -> void:
	var scene = load(path)
	var inst = scene.instantiate()
	add_child(inst)
	fade_children(inst)
	pass

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	$Music.play()
	ShowMenu("res://menu/main.tscn")
	pass # Replace with function body.

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass

func _gui_input(event: InputEvent) -> void:
	pass

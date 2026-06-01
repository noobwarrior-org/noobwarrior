extends Control

var music : Array = ["res://sounds/music/music1.ogg", "res://sounds/music/music2.ogg"]
var currentMenu : Node

func fade_children(node : Node, initialVal : float, finalVal : float) -> void:
	for child in node.get_children():
		if child is Control:
			child.modulate.a = initialVal
			var tween = create_tween()
			tween.tween_property(child, "modulate:a", finalVal, 0.5)
			
			if child.get_child_count() > 0:
				fade_in_children(child)

func fade_in_children(node : Node) -> void:
	fade_children(node, 0.0, 1.0)
				
func fade_out_children(node : Node) -> void:
	fade_children(node, 1.0, 0.0)

func ShowMenu(path : String) -> void:
	if currentMenu != null:
		fade_out_children(currentMenu)
		await get_tree().create_timer(1.0).timeout
		currentMenu.queue_free()
	var scene = load(path)
	var menu : Node = scene.instantiate()
	currentMenu = menu
	add_child(menu)
	fade_in_children(menu)

func _ready() -> void:
	var stream = load(music.pick_random())
	$Music.stream = stream
	$Music.play()
	ShowMenu("res://menu/main/main.tscn")

func _process(delta: float) -> void:
	pass

func _gui_input(event: InputEvent) -> void:
	pass

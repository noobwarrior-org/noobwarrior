-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: Overlay
-- File: init.client.lua
-- Description: Main file for HTTP Server Base
-- Started by: Hattozo
-- Started on: 8/16/2026
-- ////////////////////////////////////////////////////////////////////////////////
local UserInputService = game:GetService("UserInputService")
local ContextActionService = game:GetService("ContextActionService")
local Players = game:GetService("Players")
local Player = Players.LocalPlayer
local PlayerGui = Player:WaitForChild("PlayerGui")

local Window = require(script.Window)
local RemoteShellWindow = nil

local Gui = Instance.new("ScreenGui")
Gui.Name = "NoobWarriorOverlay"
Gui.ResetOnSpawn = false
Gui.ZIndexBehavior = Enum.ZIndexBehavior.Sibling
Gui.DisplayOrder = math.huge
Gui.Parent = PlayerGui

-- local RemoteShell = Instance.new("Frame")
-- RemoteShell.Name = "Console"
-- RemoteShell.Size = UDim2.new(0, 400, 0, 300)
-- RemoteShell.Visible = false
-- RemoteShell.Parent = Gui
-- RemoteShell:Destroy()
-- local RemoteShell = script.RemoteShell:Clone()

local function HandleAction(actionName, inputState, _inputObject)
    if actionName == "NoobWarriorRemoteShellToggle" and inputState == Enum.UserInputState.Begin then
        -- RemoteShellWindow.Visible = not Console.Visible
        print("hello")
        if RemoteShellWindow then
            print("Destroyed")
            RemoteShellWindow:Destroy()
            RemoteShellWindow = nil
        else
            print("Created")
            RemoteShellWindow = Window.new(Gui, script.RemoteShell)
            RemoteShellWindow.Title = "Remote Shell"
            RemoteShellWindow.Maximizable = false
        end
    end
end

ContextActionService:BindAction("NoobWarriorRemoteShellToggle", HandleAction, false, Enum.KeyCode.F1)
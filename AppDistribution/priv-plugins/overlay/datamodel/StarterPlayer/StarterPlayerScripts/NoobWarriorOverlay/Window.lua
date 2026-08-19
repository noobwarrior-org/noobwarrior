-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: Overlay
-- File: Window.lua
-- Description: Class for windows
-- Started by: Hattozo
-- Started on: 8/17/2026
-- ////////////////////////////////////////////////////////////////////////////////
local Window = {}
local CurrentWindows = {}

local ReplicatedStorage = game:GetService("ReplicatedStorage")
local CollectionService = game:GetService("CollectionService")
local UserInputService = game:GetService("UserInputService")
local RunService = game:GetService("RunService")
local Players = game:GetService("Players")

if not RunService:IsClient() then
	return Window
end

local Tween = require(script.Parent.Tween)

local Player = Players.LocalPlayer
local Mouse = Player:GetMouse()
local Camera = workspace.CurrentCamera

local ResizePointCursors = {
	["TopLeft"] = "rbxassetid://12413256545",
	["TopMiddle"] = "rbxassetid://12413278036",
	["TopRight"] = "rbxassetid://12413261879",
	["MiddleLeft"] = "rbxassetid://12413270772",
	["MiddleRight"] = "rbxassetid://12413270772",
	["BottomLeft"] = "rbxassetid://12413261879",
	["BottomMiddle"] = "rbxassetid://12413278036",
	["BottomRight"] = "rbxassetid://12413256545"
}

function Window.new(Gui, Frame)
	if not RunService:IsClient() then
		return error("Window.new() can only be used on a client")	
	end
	
	if not Frame then
		return error("Window.new() requires a Frame!")
	end
	
	local self = {}
	
	-- Public variables
	self.Width = Frame.Size.X.Offset
	self.Height = Frame.Size.Y.Offset
	self.MinWidth = self.Width
	self.MinHeight = self.Height
	self.MaxWidth = self.Width * 2
	self.MaxHeight = self.Height * 2
	self.X = Camera.ViewportSize.X / 2 - self.Width / 2
	self.Y = Camera.ViewportSize.Y / 2 - self.Height / 2
	self.Maximizable = true
	self.Closeable = true
	self.Resizable = true
	self.Title = "Window"
    self.Destroyed = false
	
	-- Private variables
	local Dragging = false
	local ResizePoint = nil
	
	-- Handle the traditional Roblox GUI side of things
	local WindowFrame = script.Parent.WindowFrame:Clone()
	
	-- Private functions
	local function RecalculateShadow()
		WindowFrame.Shadow.Size = UDim2.new(0, self.Width + 150, 0, self.Height + 150)
		WindowFrame.Shadow.Position = UDim2.new(0, self.X - 75, 0, self.Y - 75)
	end
	
	local function UpdateGUI()
		WindowFrame.Container.Size = UDim2.new(0, self.Width, 0, self.Height)
		WindowFrame.Container.Position = UDim2.new(0, self.X, 0, self.Y)
		RecalculateShadow()
		
		WindowFrame.Container.Topbar.Maximize.Visible = self.Maximizable
		WindowFrame.Container.Topbar.CloseButton.Visible = self.Closeable
		
		if not self.Closeable then
			WindowFrame.Container.Topbar.Maximize.Position = UDim2.new(1, 0, 0.5, 0)
		else
			WindowFrame.Container.Topbar.Maximize.Position = UDim2.new(1, -24, 0.5, 0)
		end
		
		WindowFrame.Container.Topbar.Title.Text = self.Title
	end
	
	--[[ Commented out because this might screw things up in some scenarios
	local function PlayFadeInAnimation()
		for _, object in pairs(WindowFrame:GetDescendants()) do
			if object:IsA("GuiObject") then
				local currentTransparency = object.BackgroundTransparency
				
				object.BackgroundTransparency = 1
				
				Tween(object, {"BackgroundTransparency"}, currentTransparency, self.FadeInSpeed, Enum.EasingStyle.Quad, Enum.EasingDirection.In)
			end
			
			if object:IsA("ImageLabel") or object:IsA("ImageButton") then
				local currentTransparency = object.ImageTransparency
				
				object.ImageTransparency = 1
				
				Tween(object, {"ImageTransparency"}, currentTransparency, self.FadeInSpeed, Enum.EasingStyle.Quad, Enum.EasingDirection.In)
			end
			
			if object:IsA("TextLabel") or object:IsA("TextButton") then
				local currentTransparency = object.TextTransparency
				
				object.TextTransparency = 1
				
				Tween(object, {"TextTransparency"}, currentTransparency, self.FadeInSpeed, Enum.EasingStyle.Quad, Enum.EasingDirection.In)
			end
			
			if object:IsA("UIStroke") then
				local currentTransparency = object.Transparency

				object.Transparency = 1

				Tween(object, {"Transparency"}, currentTransparency, self.FadeInSpeed, Enum.EasingStyle.Quad, Enum.EasingDirection.In)
			end
		end
	end
	
	local function PlayFadeOutAnimation()
		for _, object in pairs(WindowFrame:GetDescendants()) do
			if object:IsA("GuiObject") then
				Tween(object, {"BackgroundTransparency"}, 1, self.FadeOutSpeed, Enum.EasingStyle.Quad, Enum.EasingDirection.In)
			end
		end
	end
	]]
	
	local function Show()
		WindowFrame.Container.Visible = true
		WindowFrame.Shadow.Visible = true
	end
	
	local function Hide()
		WindowFrame.Container.Visible = false
		WindowFrame.Shadow.Visible = false
	end
	
	local function Destroy()
		WindowFrame:Destroy()
        self.Destroyed = true
	end
	
	-- Events
	WindowFrame.Container.Topbar.CloseButton.Activated:Connect(function()
		Destroy()
	end)
	
	WindowFrame.Container.Topbar.Maximize.Activated:Connect(function()
		self.X = 0
		self.Y = 32
		self.Width = Camera.ViewportSize.X
		self.Height = Camera.ViewportSize.Y - 32
	end)
	
	WindowFrame.Container.Topbar.CloseButton.MouseEnter:Connect(function()
		Tween(WindowFrame.Container.Topbar.CloseButton.Fill, {"BackgroundTransparency"}, 0.75, 0.1, Enum.EasingStyle.Quad, Enum.EasingDirection.InOut)
	end)
	
	WindowFrame.Container.Topbar.CloseButton.MouseLeave:Connect(function()
		Tween(WindowFrame.Container.Topbar.CloseButton.Fill, {"BackgroundTransparency"}, 1, 0.1, Enum.EasingStyle.Quad, Enum.EasingDirection.InOut)
	end)
	
	WindowFrame.Container.Topbar.Maximize.MouseEnter:Connect(function()
		Tween(WindowFrame.Container.Topbar.Maximize.Fill, {"BackgroundTransparency"}, 0.75, 0.1, Enum.EasingStyle.Quad, Enum.EasingDirection.InOut)
	end)

	WindowFrame.Container.Topbar.Maximize.MouseLeave:Connect(function()
		Tween(WindowFrame.Container.Topbar.Maximize.Fill, {"BackgroundTransparency"}, 1, 0.1, Enum.EasingStyle.Quad, Enum.EasingDirection.InOut)
	end)
	
	WindowFrame.Container.Topbar.MouseButton1Down:Connect(function()
		Dragging = true
	end)
	
	WindowFrame.Container.Topbar.MouseButton1Up:Connect(function()
		Dragging = false
	end)
	
	for _, ResizeButton in pairs(WindowFrame.Container.ResizePoints:GetChildren()) do
		if ResizeButton:IsA("ImageButton") then
			ResizeButton.MouseButton1Down:Connect(function()
				if self.Resizable then
					ResizePoint = ResizeButton.Name
				end
			end)
			
			ResizeButton.MouseButton1Up:Connect(function()
				if self.Resizable then
					ResizePoint = nil
				end
			end)
			
			ResizeButton.MouseEnter:Connect(function()
				if self.Resizable then
					-- AeroCursor:SetActive(true)
					-- AeroCursor:SetState(ResizePointCursors[ResizeButton.Name])
				end
			end)
			
			ResizeButton.MouseLeave:Connect(function()
				if self.Resizable then
					-- AeroCursor:SetActive(false)
				end
			end)
		end
	end
	
	local Content = Frame:Clone()
	Content.Name = "Content"
	Content.Visible = true
	Content.Size = UDim2.new(1, 0, 1, 0)
	Content.Parent = WindowFrame.Container
	
    WindowFrame.Visible = true
	WindowFrame.Parent = Gui
	
	local UpdateLoop
	
	UpdateLoop = RunService.RenderStepped:Connect(function()
		if WindowFrame:IsDescendantOf(game) then
			UpdateGUI()
		else
			UpdateLoop:Disconnect()
		end
	end)
	
	local lastMouseX, lastMouseY = Mouse.X, Mouse.Y
	local MouseMoved
	
	MouseMoved = Mouse.Move:Connect(function()
		if WindowFrame:IsDescendantOf(game) then
			local deltaX = Mouse.X - lastMouseX
			local deltaY = Mouse.Y - lastMouseY
			
			if Dragging then
				self.X += deltaX
				self.Y += deltaY
			end
			
			if ResizePoint ~= nil and self.Resizable then
				if ResizePoint == "TopLeft" then
					self.X += deltaX
					self.Y += deltaY
					self.Width += -deltaX
					self.Height += -deltaY
				end
				
				if ResizePoint == "TopMiddle" then
					self.Y += deltaY
					self.Height += -deltaY
				end
				
				if ResizePoint == "TopRight" then
					self.Y += deltaY
					self.Width += deltaX
					self.Height += -deltaY
				end
				
				if ResizePoint == "MiddleLeft" then
					self.X += deltaX
					self.Width += -deltaX
				end
				
				if ResizePoint == "MiddleRight" then
					self.Width += deltaX
				end
				
				if ResizePoint == "BottomLeft" then
					self.X += deltaX
					self.Width += -deltaX
					self.Height += deltaY
				end
				
				if ResizePoint == "BottomMiddle" then
					self.Height += deltaY
				end
				
				if ResizePoint == "BottomRight" then
					self.Width += deltaX
					self.Height += deltaY
				end
			end
			
			lastMouseX, lastMouseY = Mouse.X, Mouse.Y
		else
			MouseMoved:Disconnect()
		end
	end)
	
	table.insert(CurrentWindows, self)
	
	-- This is where we handle our own API side of things
	function self:SetPos(x, y)
		self.X = x
		self.Y = y
	end
	
	function self:SetSize(x, y)
		self.Width = x
		self.Height = y
	end
	
	function self:SetTitle(title)
		self.Title = title
	end
	
	function self:SetParent(instance)
		WindowFrame.Parent = instance
	end
	
	function self:GetPos()
		return self.X, self.Y
	end
	
	function self:GetSize()
		return self.Width, self.Height
	end
	
	function self:GetTitle()
		return self.Title
	end
	
	function self:GetParent()
		return WindowFrame.Parent
	end
	
	function self:GetGui()
		return WindowFrame
	end
	
	function self:GetContent()
		return Content
	end
	
	function self:Show()
		Show()
	end
	
	function self:Hide()
		Hide()
	end
	
	function self:Destroy()
		Destroy()
	end
	
	function self:Close()
		self:Destroy()
	end
	
	return self
end

function Window:GetCurrentWindows()
	return CurrentWindows
end

function Window:DestroyAllWindows()
	for _, window in pairs(Window:GetCurrentWindows()) do
		window:Destroy()
	end
end

function Window:CloseAllWindows()
	Window:DestroyAllWindows()
end

return Window

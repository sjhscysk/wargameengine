function VR()
	EnableVR(true, true)
end

SetLightPosition(1, 0, 0, 30)
--EnableShadowMap(1024, 53)
EnableMSAA()
SetAnisotropy(GetMaxAnisotropy())
if(GetRendererName() == "DirectX11") then
	SetShaders("directX11/gpu_skinning.hlsl", "directX11/gpu_skinning.hlsl")
	SetParticleSystemShaders("directX11/particle.hlsl", "directX11/particle.hlsl")
elseif(GetRendererName() == "OpenGLES") then
	SetShaders("GLES/gpu_skinning.vsh", "GLES/gpu_skinning.fsh")
	SetParticleSystemShaders("GLES/particle.vsh", "GLES/particle.fsh")
	UI:SetScale(2)
	Viewport:EnableTouchMode()
else
	--SetShaders("openGL/gpu_skinning.vsh", "openGL/gpu_skinning.fsh")--Slows down rendering by 50%
	--SetParticleSystemShaders("openGL/particle.vsh", "openGL/particle.fsh")
end
EnableGPUSkinning()
CreateSkybox(50, "skybox")--Creates a skybox (size in OpenGLUnits, path to texture folder (names are fixed))
CreateTable(60, 60, "sand.dds")--Creates a table (width, height, texture)
local count = 30
for i = 1, count do
	local x = -count / 2 + i
	for j = 1, count do
		local y = -count / 2 + j
		local object = Object:New("Angel_of_Death.wbm", x, y, 0)
	end
end
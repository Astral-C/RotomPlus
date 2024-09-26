#pragma once

#include "UCamera.hpp"

#include <vector>
#include <filesystem>
#include <memory>

class URotomContext {
	
	std::array<std::string, 3> mEditorTools = {"Map Editor", "Encounter Editor", "A third, funnier tab"};
	
	uint32_t mGizmoOperation { 0 };

	USceneCamera mCamera;
	
	uint32_t mMainDockSpaceID;
	uint32_t mDockNodeLeftID;
	uint32_t mDockNodeUpLeftID;
	uint32_t mDockNodeDownLeftID;
	
	bool mOptionsOpen { false };
	bool mAboutOpen { false };
	bool mViewportIsFocused { false };

	bool bIsDockingSetUp { false };
	bool bIsFileDialogOpen { false };
	bool bIsSaveDialogOpen { false };

	// Rendering surface
	uint32_t mFbo, mRbo, mViewTex, mPickTex;

	float mPrevWinWidth { -1.0f };
	float mPrevWinHeight { -1.0f };

	void RenderMainWindow(float deltaTime);
	void RenderPanels(float deltaTime);
	void RenderMenuBar();

	void LoadFromPath(std::filesystem::path filePath);

	void SaveModel(std::filesystem::path filePath);


public:
	URotomContext();
	~URotomContext();


	void HandleSelect();
	bool Update(float deltaTime);
	void Render(float deltaTime);
	USceneCamera* GetCamera() { return &mCamera; }
};
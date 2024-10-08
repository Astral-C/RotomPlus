#pragma once

#include "UCamera.hpp"

#include <vector>
#include <filesystem>
#include <memory>
#include <glm/glm/glm.hpp>

#include <NDS/System/Rom.hpp>
#include <NDS/System/Archive.hpp>
#include "Map/Map.hpp"

class URotomContext {
	
	uint32_t mCurrentMatrixIdx { 0 };
	std::string mCurrentTool = "Map Editor";
	std::array<std::string, 4> mEditorTools = {"Map Editor", "Movement Permissions", "Encounter Editor", "Trainer Editor"};
	std::vector<std::string> mLocationNames = {"(None)"};
	int mCurrentLocationIdx { 0 };
	std::string mCurrentLocation = "";
	glm::vec2 mPrevChunk = {0,0};
	glm::vec2 mSelectedChunk = {0,0};
	std::shared_ptr<MapChunk> mSelectedChunkPtr { nullptr };
	Building* mSelectedBuilding { nullptr };
	MapManager mMapManager;

	std::unique_ptr<Palkia::Nitro::Rom> mRom = nullptr;

	uint32_t mGizmoOperation { 0 };

	USceneCamera mCamera;
	
	uint32_t mMainDockSpaceID;
	uint32_t mDockNodeLeftID;
	uint32_t mDockNodeUpLeftID;
	uint32_t mDockNodeDownLeftID;
	
	bool mOptionsOpen { false };
	bool mAboutOpen { false };
	bool mViewportIsFocused { false };
	bool mPermsTooltip { false };
	uint32_t mPermsTooltipX = 0xFFFFFFFF;
	uint32_t mPermsTooltipY = 0xFFFFFFFF;
	uint32_t mSelectedX = 0;
	uint32_t mSelectedY = 0;

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

#pragma once

#include "Map/Chunk.hpp"
#include "UCamera.hpp"
#include "UAreaRenderer.hpp"

#include <vector>
#include <filesystem>
#include <memory>
#include <glm/glm/glm.hpp>

#include <NDS/System/Rom.hpp>
#include <NDS/System/Archive.hpp>
#include "Map/Map.hpp"
#include "Map/Event.hpp"
#include "UPointSpriteManager.hpp"

class URotomContext {

	uint32_t mCurrentMatrixIdx { 0 };
	std::string mCurrentTool = "Map Editor";
	std::array<std::string, 5> mEditorTools = {"Map Editor", "Area Editor", "Chunk Editor", "Encounter Editor", "Trainer Editor"};
	std::vector<std::string> mLocationNames = {"(None)"};
	int mCurrentLocationIdx { 0 };
	std::string mCurrentLocation = "";
	glm::vec2 mPrevChunk = {0,0};
	glm::vec2 mSelectedChunk = {0,0};
	bool mImportChunkModelDialog { false };
	std::shared_ptr<MapChunk> mSelectedChunkPtr { nullptr };
	std::shared_ptr<MapChunkHeader> mSelectedChunkHeader { nullptr };
	Building* mSelectedBuilding { nullptr };
	int mSelectedEventIdx { 0 };
	Event* mSelectedEvent { nullptr };
	MapManager mMapManager;

	std::unique_ptr<Palkia::Nitro::Rom> mRom = nullptr;

	uint32_t mGizmoOperation { 0 };

	USceneCamera mCamera;
	CAreaRenderer mAreaRenderer;
	CPointSpriteManager mPointRenderer;

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

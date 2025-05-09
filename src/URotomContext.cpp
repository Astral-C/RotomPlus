#include <iostream>
#include "URotomContext.hpp"

#include "../lib/ImGuiFileDialog/ImGuiFileDialog.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <glad/glad.h>
#include <bstream/bstream.h>
#include "ImGuizmo.h"
#include "IconsForkAwesome.h"
#include "Text.hpp"
#include "PokemonData.hpp"
#include <glm/glm/ext/matrix_transform.hpp>
#include <algorithm>
#include <format>
#include <vector>
#include "NDS/Assets/NSBMD.hpp"
#include "NDS/Assets/NCLR.hpp"
#include "NDS/Assets/NCGR.hpp"
#include "UPointSpriteManager.hpp"
#include "Util.hpp"
#include "GameConfig.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace {
    std::vector<CPointSprite> mBillboards {};
    std::map<uint32_t, uint32_t> mOverworldSpriteIDs {};
    std::array<uint32_t, 9> mAreaTypeImages {};
    std::array<uint32_t, 539> mPokeIcons {};
}

URotomContext::~URotomContext(){
    for(int i = 0; i < mAreaTypeImages.size(); i++){
        if(mAreaTypeImages[i] != 0xFFFFFFFF) glDeleteTextures(1, &mAreaTypeImages[i]);
    }

    glDeleteTextures(mPokeIcons.size(), &mPokeIcons[0]);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glDeleteFramebuffers(1, &mFbo);
	glDeleteRenderbuffers(1, &mRbo);
	glDeleteTextures(1, &mViewTex);
	glDeleteTextures(1, &mPickTex);
}

URotomContext::URotomContext(){
	srand(time(0));

	mAreaTypeImages.fill(0xFFFFFFFF);
	mPokeIcons.fill(0xFFFFFFFF);

	ImGuiIO& io = ImGui::GetIO();

	if(std::filesystem::exists((std::filesystem::current_path() / "res" / "NotoSansJP-Regular.otf"))){
		io.Fonts->AddFontFromFileTTF((std::filesystem::current_path() / "res" / "NotoSansJP-Regular.otf").string().c_str(), 16.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	}

	if(std::filesystem::exists((std::filesystem::current_path() / "res" / "forkawesome.ttf"))){
		static const ImWchar icons_ranges[] = { ICON_MIN_FK, ICON_MAX_16_FK, 0 };
		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		icons_config.GlyphMinAdvanceX = 16.0f;
		io.Fonts->AddFontFromFileTTF((std::filesystem::current_path() / "res" / "forkawesome.ttf").string().c_str(), icons_config.GlyphMinAdvanceX, &icons_config, icons_ranges );
	}

	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	mGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;

	//Palkia::Formats::NSBMD test;
	//test.LoadIMD("test.imd");

	mAreaRenderer.Init();
}

bool URotomContext::Update(float deltaTime) {
	if(mViewportIsFocused){
		mCamera.Update(deltaTime);

		if(ImGui::IsKeyPressed(ImGuiKey_1)){
			mGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
		}

		if(ImGui::IsKeyPressed(ImGuiKey_2)){
			mGizmoOperation = ImGuizmo::OPERATION::ROTATE;
		}

		if(ImGui::IsKeyPressed(ImGuiKey_3)){
			mGizmoOperation = ImGuizmo::OPERATION::SCALE;
		}

		if(ImGui::IsKeyPressed(ImGuiKey_Escape)){
			// deselect
		}

		if(ImGui::IsKeyPressed(ImGuiKey_O)){
			mCamera.ToggleOrtho();
		}
	}

	return true;
}

void URotomContext::Render(float deltaTime) {
	ImGuiIO& io = ImGui::GetIO();

	RenderMenuBar();

	const ImGuiViewport* mainViewport = ImGui::GetMainViewport();

	ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar | ImGuiDockNodeFlags_NoDockingInCentralNode;
	mMainDockSpaceID = ImGui::DockSpaceOverViewport(0, mainViewport, dockFlags);

	if(!bIsDockingSetUp){

		glGenFramebuffers(1, &mFbo);
		glBindFramebuffer(GL_FRAMEBUFFER, mFbo);

		glGenRenderbuffers(1, &mRbo);
		glBindRenderbuffer(GL_RENDERBUFFER, mRbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1280, 720);

		glGenTextures(1, &mViewTex);
		glGenTextures(1, &mPickTex);

		glBindTexture(GL_TEXTURE_2D, mViewTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1280, 720, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

		glBindTexture(GL_TEXTURE_2D, mPickTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, 1280, 720, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mViewTex, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mPickTex, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mRbo);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);


		ImGui::DockBuilderRemoveNode(mMainDockSpaceID); // clear any previous layout
		ImGui::DockBuilderAddNode(mMainDockSpaceID, dockFlags | ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(mMainDockSpaceID, mainViewport->Size);


		mDockNodeLeftID = ImGui::DockBuilderSplitNode(mMainDockSpaceID, ImGuiDir_Left, 0.20f, nullptr, &mMainDockSpaceID);
		mDockNodeDownLeftID = ImGui::DockBuilderSplitNode(mDockNodeLeftID, ImGuiDir_Down, 0.5f, nullptr, &mDockNodeUpLeftID);


		ImGui::DockBuilderDockWindow("zoneWindow", mDockNodeUpLeftID);
		ImGui::DockBuilderDockWindow("chunkWindow", ImGui::DockBuilderSplitNode(mDockNodeUpLeftID, ImGuiDir_Down, 0.5f, nullptr, nullptr));
		ImGui::DockBuilderDockWindow("viewportWindow", mMainDockSpaceID);

		ImGui::DockBuilderFinish(mMainDockSpaceID);
		bIsDockingSetUp = true;
	}


	ImGuiWindowClass mainWindowOverride;
	mainWindowOverride.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
	ImGui::SetNextWindowClass(&mainWindowOverride);

	ImGui::Begin("zoneWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
	ImGui::Text("Zones");
		ImGui::Separator();

		// Render zones as listed by names list
		if(ImGui::BeginCombo("##locations", mLocationNames[mCurrentLocationIdx].data())){
			for(uint32_t i = 0; i < mLocationNames.size(); i++){
					bool is_selected = (mCurrentLocationIdx == i);
					if (ImGui::Selectable(mLocationNames[i].data(), is_selected)){
						mCurrentLocation = mLocationNames[i];
						mCurrentMatrixIdx = 0;
						mSelectedBuilding = nullptr;
						mMapManager.LoadZone(i);
						mCurrentLocationIdx = i;
					}
					if (is_selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::Separator();
		ImGui::NewLine();
		ImGui::NewLine();

		ImGui::Text("Map Matrices");
		ImGui::Separator();
		auto matrices = mMapManager.GetMatrices();
		if(matrices.size() > 0){
			if(ImGui::BeginCombo("##mapMatrices", matrices[mCurrentMatrixIdx]->GetName().data())){
				for(int i = 0; i < matrices.size(); i++){
					bool is_selected = (mCurrentMatrixIdx == i); // You can store your selection however you want, outside or inside your objects
					if (ImGui::Selectable(std::format("{} : {}", i, matrices[i]->GetName()).data(), is_selected)){
						mMapManager.SetActiveMatrix(i);
						mCurrentMatrixIdx = i;
						mSelectedChunkPtr = nullptr;
					}
					if (is_selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			if(ImGui::Button("Save Current Matrix")){
				mMapManager.SaveMatrix();
			}
		}


	ImGui::End();

	ImGui::SetNextWindowClass(&mainWindowOverride);
	ImGui::Begin("chunkWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
		ImGui::Text("Current Map");
		ImGui::Separator();

		ImGui::Text("Overworld Events");
		if(ImGui::BeginCombo("##mapEventsOverworld", std::format("Event {}", mSelectedEventIdx).data())){
			for(uint32_t i = 0; i < mMapManager.mEvents.overworldEvents.size(); i++){
					bool is_selected = (&mMapManager.mEvents.overworldEvents[i] == mSelectedEvent);
					if (ImGui::Selectable(std::format("Event {}", i).data(), is_selected)){
						mSelectedEvent = &mMapManager.mEvents.overworldEvents[i];
						mSelectedEventIdx = i;
					}
					if (is_selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		if(mSelectedEvent != nullptr){
			if(mSelectedEvent->eventType == EventType::Overworld){
				int32_t ovId = (int32_t)reinterpret_cast<Overworld*>(mSelectedEvent)->overlayID;
				int32_t orientation = (int32_t)reinterpret_cast<Overworld*>(mSelectedEvent)->orientation;
				ImGui::InputInt("Sprite ID", &ovId);
				ImGui::InputInt("Orientation", &orientation);
				reinterpret_cast<Overworld*>(mSelectedEvent)->orientation = (uint16_t)orientation;
				reinterpret_cast<Overworld*>(mSelectedEvent)->overlayID = (uint16_t)ovId;
			}

			if(mSelectedEvent->eventType == EventType::Warp){
				auto chunkHeaders = mMapManager.GetChunkHeaders();
				uint16_t targetHeaderIdx = reinterpret_cast<Warp*>(mSelectedEvent)->targetHeader;
				auto targetHeader = chunkHeaders[targetHeaderIdx];

				ImGui::Text("Target Map");
				std::string curName = std::format("{} : {}", mLocationNames[targetHeader->mPlaceNameID], mMapManager.GetChunkName(targetHeaderIdx));
				if(ImGui::BeginCombo("##warpTargetCombo", curName.data())){
					for(uint16_t i = 0; i < (uint16_t)chunkHeaders.size(); i++){
							std::string targetName = std::format("{} : {}", mLocationNames[chunkHeaders[i]->mPlaceNameID], mMapManager.GetChunkName(i));
							bool is_selected = (curName == targetName);
							if (ImGui::Selectable(targetName.data(), is_selected)){
								reinterpret_cast<Warp*>(mSelectedEvent)->targetHeader = i;
							}
							if (is_selected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				ImGui::Text("Anchor");
				ImGui::SameLine();
				int32_t anchor = reinterpret_cast<Warp*>(mSelectedEvent)->anchor;
				ImGui::InputInt("##warpAnchor", &anchor);
				reinterpret_cast<Warp*>(mSelectedEvent)->anchor = (uint16_t)anchor;

			}
		}
	ImGui::End();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
	ImGui::SetNextWindowClass(&mainWindowOverride);
	ImGui::Begin("viewportWindow");

		ImGui::BeginTabBar("mapToolbar", ImGuiTabBarFlags_Reorderable);
			for(auto tool : mEditorTools){
				if(ImGui::BeginTabItem(tool.data())){
					mCurrentTool = tool;
					ImGui::EndTabItem();
				}
			}
		ImGui::EndTabBar();

		if(mCurrentTool == "Map Editor"){
			ImVec2 winSize = ImGui::GetContentRegionAvail();
			ImVec2 cursorPos = ImGui::GetCursorScreenPos();

			glBindFramebuffer(GL_FRAMEBUFFER, mFbo);

			if(winSize.x != mPrevWinWidth || winSize.y != mPrevWinHeight){
				glDeleteTextures(1, &mViewTex);
				glDeleteTextures(1, &mPickTex);
				glDeleteRenderbuffers(1, &mRbo);

				glGenRenderbuffers(1, &mRbo);
				glBindRenderbuffer(GL_RENDERBUFFER, mRbo);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (uint32_t)winSize.x, (uint32_t)winSize.y);

				glGenTextures(1, &mViewTex);
				glGenTextures(1, &mPickTex);

				glBindTexture(GL_TEXTURE_2D, mViewTex);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (uint32_t)winSize.x, (uint32_t)winSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				glBindTexture(GL_TEXTURE_2D, mPickTex);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, (uint32_t)winSize.x, (uint32_t)winSize.y, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mViewTex, 0);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mPickTex, 0);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mRbo);

				GLenum attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
				glDrawBuffers(2, attachments);

			}

			glViewport(0, 0, (uint32_t)winSize.x, (uint32_t)winSize.y);


			glClearColor(0.100f, 0.261f, 0.402f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			int32_t unused = 0;
			glClearTexImage(mPickTex, 0, GL_RED_INTEGER, GL_INT, &unused);

			mPrevWinWidth = winSize.x;
			mPrevWinHeight = winSize.y;

			glm::mat4 projection, view;
			projection = mCamera.GetProjectionMatrix();
			view = mCamera.GetViewMatrix();

			// Render Models
			glEnable(GL_DEPTH_TEST);
			mMapManager.Draw(projection * view);
			mPointRenderer.Draw(&mCamera);

			std::size_t billboardsSize = mBillboards.size();

			for(auto sprite : mMapManager.mEvents.overworldEvents){
				//RenderEvent(projection * view * glm::translate(glm::mat4(1.0f), glm::vec3(((sprite.x)*16)-256+8, Palkia::fixed(sprite.z)+8, ((sprite.y)*16)-256+8)), sprite.id, sprite.spriteID);
				if(sprite.sprite == nullptr){
				    // get sprite ID
                    mBillboards.push_back({glm::vec3(((sprite.x)*16)-256+8, Palkia::fixed(sprite.z) + ((mMapManager.GetActiveMatrix()->GetEntries()[(((sprite.y + 128) / 256) * mMapManager.GetActiveMatrix()->GetWidth()) + ((sprite.x + 128) / 256)].mHeight+1)*8), ((sprite.y)*16)-256+8), 12000, mOverworldSpriteIDs[sprite.overlayID], 1, static_cast<int>(sprite.id)});
                    sprite.sprite = &mBillboards.back();
				}
			}

			if(mBillboards.size() != billboardsSize){
                mPointRenderer.UpdateData(mBillboards);
			}

			for(auto warp : mMapManager.mEvents.warpEvents){
				glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(((warp.x)*16)-256+8, ((Palkia::fixed(warp.height)+1)*16) + ((mMapManager.GetActiveMatrix()->GetEntries()[(((warp.y + 128) / 256) * mMapManager.GetActiveMatrix()->GetWidth()) + ((warp.x + 128) / 256)].mHeight+1)*8), ((warp.y)*16)-256+8));
				m = glm::scale(m, glm::vec3(0.025f, 0.025f, 0.025f));
				mAreaRenderer.DrawShape(&mCamera, AreaRenderShape::BOX_BASE, warp.id, m, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
				//RenderEvent(projection * view * glm::translate(glm::mat4(1.0f), glm::vec3(((sprite.x)*16)-256, Palkia::fixed(sprite.z), ((sprite.y+1)*16)-256)));
			}

			cursorPos = ImGui::GetCursorScreenPos();
			ImGui::Image(static_cast<uintptr_t>(mViewTex), { winSize.x, winSize.y }, {0.0f, 1.0f}, {1.0f, 0.0f});

			if(ImGui::IsWindowFocused()){
				mViewportIsFocused = true;
			} else {
				mViewportIsFocused = false;
			}

			if(ImGui::IsItemClicked(0) && !ImGuizmo::IsOver()){
				ImVec2 mousePos = ImGui::GetMousePos();

				ImVec2 pickPos = {
					mousePos.x - cursorPos.x,
					winSize.y - (mousePos.y - cursorPos.y)
				};

				glPixelStorei(GL_PACK_ALIGNMENT, 1);
				glReadBuffer(GL_COLOR_ATTACHMENT1);
				uint32_t id = 0xFFFFFFFF;
				glReadPixels(static_cast<GLint>(pickPos.x), static_cast<GLint>(pickPos.y), 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, (void*)&id);

				std::cout << "ID Select was " << std::hex << id << std::dec << std::endl;

				if(id != 0){
					bool selected = false;
					for (std::size_t i = 0; i < mMapManager.mEvents.overworldEvents.size(); i++){
						if(id == mMapManager.mEvents.overworldEvents[i].id){
							mSelectedEvent = &mMapManager.mEvents.overworldEvents[i];
							mSelectedBuilding = nullptr;
							selected = true;
						}
					}
					for (std::size_t i = 0; i < mMapManager.mEvents.warpEvents.size(); i++){
						if(id == mMapManager.mEvents.warpEvents[i].id){
							mSelectedEvent = &mMapManager.mEvents.warpEvents[i];
							mSelectedBuilding = nullptr;
							selected = true;
						}
					}
					if(!selected && mMapManager.GetActiveMatrix() != nullptr){
						auto result = mMapManager.GetActiveMatrix()->Select(id);
						if(result.first != nullptr){
							mSelectedBuilding = result.first;
							mSelectedChunk.x = result.second.first;
							mSelectedChunk.y = result.second.second;
							mSelectedEvent = nullptr;
							selected = true;
						}
					}
				} else {
					mSelectedBuilding = nullptr;
					mSelectedEvent = nullptr;
				}


			}

			ImGuizmo::BeginFrame();
			ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
			ImGuizmo::SetRect(cursorPos.x, cursorPos.y, winSize.x, winSize.y);

			if(mSelectedBuilding != nullptr){
				glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(mSelectedBuilding->x + (512 * mSelectedChunk.x), mSelectedBuilding->y, mSelectedBuilding->z + (512 * mSelectedChunk.y)));
				glm::mat4 delta(1.0f);
				if(ImGuizmo::Manipulate(&mCamera.GetViewMatrix()[0][0], &mCamera.GetProjectionMatrix()[0][0], (ImGuizmo::OPERATION)mGizmoOperation, ImGuizmo::WORLD, &transform[0][0], &delta[0][0])){
					glm::vec4 newPos = transform[3];
					mSelectedBuilding->x = newPos.x - (512 * mSelectedChunk.x);
					mSelectedBuilding->y = newPos.y;
					mSelectedBuilding->z = newPos.z - (512 * mSelectedChunk.y);

				}

			} else if(mSelectedEvent != nullptr){
				if(mSelectedEvent->eventType == EventType::Overworld){
					Overworld* event = reinterpret_cast<Overworld*>(mSelectedEvent);
					glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(((event->x)*16)-256+8, Palkia::fixed(event->z)+8, ((event->y)*16)-256+8));
					glm::mat4 delta(1.0f);
					if(ImGuizmo::Manipulate(&mCamera.GetViewMatrix()[0][0], &mCamera.GetProjectionMatrix()[0][0], (ImGuizmo::OPERATION)mGizmoOperation, ImGuizmo::WORLD, &transform[0][0], &delta[0][0])){
						glm::vec4 newPos = transform[3];
						event->x = ((newPos.x - 8 + 256) / 16);
						event->y = ((newPos.z - 8 + 256) / 16);
						event->z = (uint32_t)((newPos.y-8) * (1 << 12));
						event->sprite->Position = glm::vec3(event->x, event->y, event->z);
						mPointRenderer.UpdateData(mBillboards);
					}
				} else if(mSelectedEvent->eventType == EventType::Warp){
					Warp* event = reinterpret_cast<Warp*>(mSelectedEvent);
					glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(((event->x)*16)-256+8, Palkia::fixed(event->height)+8, ((event->y)*16)-256+8));
					glm::mat4 delta(1.0f);
					if(ImGuizmo::Manipulate(&mCamera.GetViewMatrix()[0][0], &mCamera.GetProjectionMatrix()[0][0], (ImGuizmo::OPERATION)mGizmoOperation, ImGuizmo::WORLD, &transform[0][0], &delta[0][0])){
						glm::vec4 newPos = transform[3];
						event->x = ((newPos.x - 8 + 256) / 16);
						event->y = ((newPos.z - 8 + 256) / 16);
						event->height = (uint32_t)((newPos.y-8) * (1 << 12));
					}
				}
			}

			// gizmo operation on selected
			//if(ImGuizmo::Manipulate(&mCamera.GetViewMatrix()[0][0], &mCamera.GetProjectionMatrix()[0][0], (ImGuizmo::OPERATION)mGizmoOperation, ImGuizmo::WORLD, &transform[0][0], &delta[0][0])){
			//		object->mTransform = glm::inverse(zoneTransform) * transform;
			//}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		float scroll = 0.0f;
		if(mCurrentTool == "Chunk Editor" && mRom != nullptr){

		    if(mSelectedChunkPtr != nullptr && mSelectedChunkHeader != nullptr){
				//Ow
				ImGuiStyle& style = ImGui::GetStyle();
				float padY = style.FramePadding.y;
				style.FramePadding.y = 15;

				ImGui::SetNextItemWidth(200);
				if(ImGui::BeginCombo("##areaWinGraSelect", nullptr, ImGuiComboFlags_CustomPreview)){
                    for(uint32_t i = 0; i < mAreaTypeImages.size(); i++){
								bool is_selected = (mSelectedChunkHeader->mTextBoxType == i);
								float cursorX = ImGui::GetCursorPosX();
								if (ImGui::Selectable(std::format("##areaWin{}",i).c_str(), is_selected, 0, {136, 40})){
								    mSelectedChunkHeader->mTextBoxType = i;
								}
								ImGui::SameLine();
								ImGui::SetCursorPosX(cursorX);
								ImGui::Image(mAreaTypeImages[i], {136, 40});
								if (is_selected) ImGui::SetItemDefaultFocus();
						}
                    ImGui::EndCombo();
				}

				if(ImGui::BeginComboPreview()){
				    ImGui::SetCursorPosY(ImGui::GetCursorPosY()-15); // hate
                    ImGui::Image(mAreaTypeImages[mSelectedChunkHeader->mTextBoxType], {136, 40});
                    ImGui::EndComboPreview();
				}
				style.FramePadding.y  = padY;
            }
			if(mSelectedChunkPtr != nullptr){

				ImGui::BeginGroup();
				if(ImGui::Button("Import Chunk Model; (*.nsbmd)")){
					mImportChunkModelDialog = true;
				}

				if (mImportChunkModelDialog) {
					IGFD::FileDialogConfig config;
					ImGuiFileDialog::Instance()->OpenDialog("ImportMapNSBMDDialog", "Import Map Chunk Model", ".nsbmd", config);
				}

				if (ImGuiFileDialog::Instance()->Display("ImportMapNSBMDDialog")) {
					if (ImGuiFileDialog::Instance()->IsOk()) {
						std::string FilePath = ImGuiFileDialog::Instance()->GetFilePathName();
						std::cout << FilePath << std::endl;

						mSelectedChunkPtr->ImportChunkNSBMD(FilePath);
						mMapManager.ReloadGraphics();
						mMapManager.SaveMatrix();

						mImportChunkModelDialog = false;
					} else {
						mImportChunkModelDialog = false;
					}

					ImGuiFileDialog::Instance()->Close();
				}

				ImGui::EndGroup();
			}

			ImVec2 padding = ImGui::GetStyle().CellPadding;
			ImGui::GetStyle().CellPadding = ImVec2(0.0f, 0.0f);
			ImGui::BeginChild("##movementPositions");
			scroll = ImGui::GetScrollMaxY() - ImGui::GetScrollY();
			if(mSelectedChunkPtr == nullptr && mMapManager.GetActiveMatrix() != nullptr){
				auto entries = mMapManager.GetActiveMatrix()->GetEntries();

				if(mMapManager.GetActiveMatrix()->GetHeight() == 1 && mMapManager.GetActiveMatrix()->GetWidth() == 1){
					mSelectedChunkPtr = entries[0].mChunk;
					mSelectedChunkHeader = entries[0].mChunkHeader.lock();
				}

				ImGui::BeginTable("##mapMatrixView", mMapManager.GetActiveMatrix()->GetWidth(), ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX);

				for(std::size_t y = 0; y < mMapManager.GetActiveMatrix()->GetHeight(); y++){
					for(std::size_t x = 0; x < mMapManager.GetActiveMatrix()->GetWidth(); x++){
						if(entries[(y * mMapManager.GetActiveMatrix()->GetWidth()) + x].mChunk != nullptr){
							bool isInLocation = mLocationNames[entries[(y * mMapManager.GetActiveMatrix()->GetWidth()) + x].mChunkHeader.lock()->mPlaceNameID] == mCurrentLocation;

							if(isInLocation) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4, 0.8, 0.5, 1.0));
							if(ImGui::Button(std::format("{}, {}", x, y).c_str(), ImVec2(-1.0,0))){
								mSelectedChunkPtr = entries[(y * mMapManager.GetActiveMatrix()->GetWidth()) + x].mChunk;
								mSelectedChunkHeader = entries[(y * mMapManager.GetActiveMatrix()->GetWidth()) + x].mChunkHeader.lock(); // huh??
							}
							if(isInLocation) ImGui::PopStyleColor();

						}
						ImGui::TableNextColumn();
					}
					ImGui::TableNextRow();
				}

				ImGui::EndTable();

			} else if(mMapManager.GetActiveMatrix() != nullptr) {
				if(mMapManager.GetActiveMatrix()->GetHeight() > 1 && mMapManager.GetActiveMatrix()->GetHeight() > 1){
					if(ImGui::Button(ICON_FK_BACKWARD " Back", ImVec2(-1, 0))){
						mSelectedChunkPtr = nullptr;
						mSelectedChunkHeader = nullptr;
					}
				}
				if(mSelectedChunkPtr != nullptr){
					ImGui::BeginTable("##mapMatrixView", 32, ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX);
					for(std::size_t y = 0; y < 32; y++){
						ImGui::TableNextRow();
						for(std::size_t x = 0; x < 32; x++){
							ImGui::TableNextColumn();
							auto perms = mSelectedChunkPtr->GetMovementPermissions()[(y * 32) + x];
							bool walkable = perms.second == 0x80;
							if(walkable) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0, 0.05, 0.05, 1.0));


							if(ImGui::Button(std::format("{:x}##{},{},{},{}", perms.first, x, y, y, x).c_str(), ImVec2(-1.0, 0.0))){
								//Show a tooltip with a combobox
								mPermsTooltip = true;
								mPermsTooltipX = ImGui::GetCursorScreenPos().x - (ImGui::GetItemRectSize().x / 2);
								mPermsTooltipY = ImGui::GetCursorScreenPos().y - (ImGui::GetItemRectSize().y * 2);
								mSelectedX = x;
								mSelectedY = y;
							}

							if(ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)){
								mSelectedChunkPtr->GetMovementPermissions()[(y * 32) + x].second = mSelectedChunkPtr->GetMovementPermissions()[(y * 32) + x].second == 0x80 ? 0x00 : 0x80;
							}
							if(walkable) ImGui::PopStyleColor();
						}
					}
					ImGui::EndTable();
				}
			}
			ImGui::EndChild();
			ImGui::GetStyle().CellPadding = padding;
		}

		if(mCurrentTool == "Trainer Editor" && mRom != nullptr){

		}

		if(mCurrentTool == "Area Editor" && mRom != nullptr){

		}

		if(mCurrentTool == "Encounter Editor" && mRom != nullptr){
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {5, 5});


			// its sucgh a mess :<
			auto windowWidth = ImGui::GetWindowSize().x;
			if(mRom->GetHeader().gameCode == (uint32_t)'EGPI'){
				ImGui::BeginTable("##encounterData", 7, ImGuiTableFlags_Borders);
				ImGui::TableSetupColumn("Morning");
				ImGui::TableSetupColumn("Day");
				ImGui::TableSetupColumn("Night");
				ImGui::TableSetupColumn("Surfing");
				ImGui::TableSetupColumn("Old Rod");
				ImGui::TableSetupColumn("Good Rod");
				ImGui::TableSetupColumn("Super Rod");
				ImGui::TableHeadersRow();

				std::array<int, 12> slotTitles = {20, 20, 10, 10, 10, 10, 5, 5, 4, 4, 1, 1};
				std::array<int, 5> fiveSlotTitles = {20, 10, 5, 4, 1};

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TableNextColumn();
				ImGui::Text("Rate");
				ImGui::SameLine();
				ImGui::InputInt("##walkingEncounterRate", (int*)&mMapManager.mEncounters.mWalkingEncounterRate);
				ImGui::TableNextColumn();
				ImGui::TableNextColumn();

				ImGui::Text("Rate");
				ImGui::SameLine();
				ImGui::InputInt("##surfingEncounterRate", (int*)&mMapManager.mEncounters.mSurfEncounterRate);
				ImGui::TableNextColumn();

				ImGui::Text("Rate");
				ImGui::SameLine();
				ImGui::InputInt("##oldRodEncounterRate", (int*)&mMapManager.mEncounters.mOldRodRate);
				ImGui::TableNextColumn();

				ImGui::Text("Rate");
				ImGui::SameLine();
				ImGui::InputInt("##goodRodEncounterRate", (int*)&mMapManager.mEncounters.mGoodRodRate);
				ImGui::TableNextColumn();

				ImGui::Text("Rate");
				ImGui::SameLine();
				ImGui::InputInt("##superRodEncounterRate", (int*)&mMapManager.mEncounters.mSuperRodRate);

				for(int x = 0; x < 12; x++){
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text(std::format("{}%%", slotTitles[x]).data());
					ImGui::SameLine();
					if(ImGui::BeginCombo(std::format("##pokeSlot{}Morning", x).data(), PokemonNames[mMapManager.mEncounters.mMorningPokemon[x]].data())){
						for(uint32_t i = 0; i < PokemonNames.size(); i++){
								bool is_selected = (mMapManager.mEncounters.mMorningPokemon[x] == i);
								if (ImGui::Selectable(PokemonNames[i].data(), is_selected)){
									mMapManager.mEncounters.mMorningPokemon[x] = i;
								}
								if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}

					ImGui::TableNextColumn();
					ImGui::Text(std::format("{}%%", slotTitles[x]).data());
					ImGui::SameLine();
					if(ImGui::BeginCombo(std::format("##pokeSlot{}Day", x).data(), PokemonNames[mMapManager.mEncounters.mDayPokemon[x]].data())){
						for(uint32_t i = 0; i < PokemonNames.size(); i++){
								bool is_selected = (mMapManager.mEncounters.mDayPokemon[x] == i);
								if (ImGui::Selectable(PokemonNames[i].data(), is_selected)){
									mMapManager.mEncounters.mDayPokemon[x] = i;
								}
								if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}

					ImGui::Text("Level");
					ImGui::SameLine();
					ImGui::InputInt(std::format("##pokeSlot{}WalkLevel", x).data(), (int*)&mMapManager.mEncounters.mWalkingLevel[x]);
					ImGui::NewLine();

					ImGui::TableNextColumn();
					ImGui::Text(std::format("{}%%", slotTitles[x]).data());
					ImGui::SameLine();
					if(ImGui::BeginCombo(std::format("##pokeSlot{}Night", x).data(), PokemonNames[mMapManager.mEncounters.mNightPokemon[x]].data())){
						for(uint32_t i = 0; i < PokemonNames.size(); i++){
								bool is_selected = (mMapManager.mEncounters.mNightPokemon[x] == i);
								if (ImGui::Selectable(PokemonNames[i].data(), is_selected)){
									mMapManager.mEncounters.mNightPokemon[x] = i;
								}
								if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}

					if(x < 5){
						ImGui::TableNextColumn();
						ImGui::Text(std::format("{}%%", fiveSlotTitles[x]).data());
						ImGui::SameLine();
						if(ImGui::BeginCombo(std::format("##pokeSlot{}Surf", x).data(), PokemonNames[mMapManager.mEncounters.mSurf[x]].data())){
							for(uint32_t i = 0; i < PokemonNames.size(); i++){
									bool is_selected = (mMapManager.mEncounters.mSurf[x] == i);
									if (ImGui::Selectable(PokemonNames[i].data(), is_selected)){
										mMapManager.mEncounters.mSurf[x] = i;
									}
									if (is_selected) ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}

						ImGui::Text("Max Level");
						ImGui::SameLine();
						ImGui::InputInt(std::format("##pokeSlot{}SurfMaxLevel", x).data(), (int*)&mMapManager.mEncounters.mSurfMaxLevels[x]);

						ImGui::Text("Min Level");
						ImGui::SameLine();
						ImGui::InputInt(std::format("##pokeSlot{}SurfMinLevel", x).data(), (int*)&mMapManager.mEncounters.mSurfMinLevels[x]);

						ImGui::TableNextColumn();
						ImGui::Text(std::format("{}%%", fiveSlotTitles[x]).data());
						ImGui::SameLine();
						if(ImGui::BeginCombo(std::format("##pokeSlot{}OldRod", x).data(), PokemonNames[mMapManager.mEncounters.mOldRod[x]].data())){
							for(uint32_t i = 0; i < PokemonNames.size(); i++){
									bool is_selected = (mMapManager.mEncounters.mOldRod[x] == i);
									if (ImGui::Selectable(PokemonNames[i].data(), is_selected)){
										mMapManager.mEncounters.mOldRod[x] = i;
									}
									if (is_selected) ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}

						ImGui::Text("Max Level");
						ImGui::SameLine();
						ImGui::InputInt(std::format("##pokeSlot{}OldRodMaxLevel", x).data(), (int*)&mMapManager.mEncounters.mOldMaxLevels[x]);

						ImGui::Text("Min Level");
						ImGui::SameLine();
						ImGui::InputInt(std::format("##pokeSlot{}OldRodMinLevel", x).data(), (int*)&mMapManager.mEncounters.mOldMinLevels[x]);

						ImGui::TableNextColumn();
						ImGui::Text(std::format("{}%%", fiveSlotTitles[x]).data());
						ImGui::SameLine();
						if(ImGui::BeginCombo(std::format("##pokeSlot{}GoodRod", x).data(), PokemonNames[mMapManager.mEncounters.mGoodRod[x]].data())){
							for(uint32_t i = 0; i < PokemonNames.size(); i++){
									bool is_selected = (mMapManager.mEncounters.mGoodRod[x] == i);
									if (ImGui::Selectable(PokemonNames[i].data(), is_selected)){
										mMapManager.mEncounters.mGoodRod[x] = i;
									}
									if (is_selected) ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}

						ImGui::Text("Max Level");
						ImGui::SameLine();
						ImGui::InputInt(std::format("##pokeSlot{}GoodRodMaxLevel", x).data(), (int*)&mMapManager.mEncounters.mGoodRodMaxLevels[x]);

						ImGui::Text("Min Level");
						ImGui::SameLine();
						ImGui::InputInt(std::format("##pokeSlot{}GoodRodMinLevel", x).data(), (int*)&mMapManager.mEncounters.mGoodRodMinLevels[x]);

						ImGui::TableNextColumn();
						ImGui::Text(std::format("{}%%", fiveSlotTitles[x]).data());
						ImGui::SameLine();
						if(ImGui::BeginCombo(std::format("##pokeSlot{}SuperRod", x).data(), PokemonNames[mMapManager.mEncounters.mSuperRod[x]].data())){
							for(uint32_t i = 0; i < PokemonNames.size(); i++){
									bool is_selected = (mMapManager.mEncounters.mSuperRod[x] == i);
									if (ImGui::Selectable(PokemonNames[i].data(), is_selected)){
										mMapManager.mEncounters.mSuperRod[x] = i;
									}
									if (is_selected) ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}

						ImGui::Text("Max Level");
						ImGui::SameLine();
						ImGui::InputInt(std::format("##pokeSlot{}SuperRodMaxLevel", x).data(), (int*)&mMapManager.mEncounters.mSuperRodMaxLevels[x]);

						ImGui::Text("Min Level");
						ImGui::SameLine();
						ImGui::InputInt(std::format("##pokeSlot{}SuperRodMinLevel", x).data(), (int*)&mMapManager.mEncounters.mSuperRodMinLevels[x]);

					} else {
						for(int i = 0; i < 4; i++) ImGui::TableNextColumn();
					}
					//PokemonNames[mMapManager.mEncounters.mWalkingEncounters[x]].data()
				}

				ImGui::EndTable();

				auto textWidth = ImGui::CalcTextSize("Special Encounters").x;

				ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
				ImGui::Text("Special Encounters");

				ImGui::BeginTable("##specialEncounterData", 2, ImGuiTableFlags_Borders);
				ImGui::TableSetupColumn("Swarm");
				ImGui::TableSetupColumn("Rock Smash");
				ImGui::TableHeadersRow();
				for(int x = 0; x < 5; x++){
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					if(x < 4){
						ImGui::Text("20%%");
						ImGui::SameLine();
						if(ImGui::BeginCombo(std::format("##pokeSlot{}Swarm", x).data(), PokemonNames[mMapManager.mEncounters.mSwarmPokemon[x]].data())){
							for(uint32_t i = 0; i < PokemonNames.size(); i++){
									bool is_selected = (mMapManager.mEncounters.mSwarmPokemon[x] == i);
									if (ImGui::Selectable(PokemonNames[i].data(), is_selected)){
										mMapManager.mEncounters.mSwarmPokemon[x] = i;
									}
									if (is_selected) ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}
					}

					ImGui::TableNextColumn();

					ImGui::Text("Min Level");
					ImGui::SameLine();
					ImGui::InputInt(std::format("##pokeSlot{}RockSmashMinLevel", x).data(), (int*)&mMapManager.mEncounters.mRockSmashMinLevels[x]);

					ImGui::Text("Max Level");
					ImGui::SameLine();
					ImGui::InputInt(std::format("##pokeSlot{}RockSmashMaxLevel", x).data(), (int*)&mMapManager.mEncounters.mRockSmashMaxLevels[x]);

					if(ImGui::BeginCombo(std::format("##pokeSlot{}RockSmash", x).data(), PokemonNames[mMapManager.mEncounters.mRockSmashPokemon[x]].data())){
						for(uint32_t i = 0; i < PokemonNames.size(); i++){
								bool is_selected = (mMapManager.mEncounters.mRockSmashPokemon[x] == i);
								if (ImGui::Selectable(PokemonNames[i].data(), is_selected)){
									mMapManager.mEncounters.mRockSmashPokemon[x] = i;
								}
								if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
				}
				ImGui::EndTable();
			} else {
				ImGui::BeginTable("##encounterData", 5, ImGuiTableFlags_Borders);
				ImGui::TableSetupColumn("Walking");
				ImGui::TableSetupColumn("Surfing");
				ImGui::TableSetupColumn("Old Rod");
				ImGui::TableSetupColumn("Good Rod");
				ImGui::TableSetupColumn("Super Rod");
				ImGui::TableHeadersRow();

				std::array<int, 12> slotTitles = {20, 20, 10, 10, 10, 10, 5, 5, 4, 4, 1, 1};
				std::array<int, 5> fiveSlotTitles = {20, 10, 5, 4, 1};

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("Rate");
				ImGui::SameLine();
				ImGui::InputInt("##walkingEncounterRate", (int*)&mMapManager.mEncounters.mWalkingEncounterRate);
				ImGui::TableNextColumn();

				ImGui::Text("Rate");
				ImGui::SameLine();
				ImGui::InputInt("##surfingEncounterRate", (int*)&mMapManager.mEncounters.mSurfEncounterRate);
				ImGui::TableNextColumn();

				ImGui::Text("Rate");
				ImGui::SameLine();
				ImGui::InputInt("##oldRodEncounterRate", (int*)&mMapManager.mEncounters.mOldRodRate);
				ImGui::TableNextColumn();

				ImGui::Text("Rate");
				ImGui::SameLine();
				ImGui::InputInt("##goodRodEncounterRate", (int*)&mMapManager.mEncounters.mGoodRodRate);
				ImGui::TableNextColumn();

				ImGui::Text("Rate");
				ImGui::SameLine();
				ImGui::InputInt("##superRodEncounterRate", (int*)&mMapManager.mEncounters.mSuperRodRate);


				for(int x = 0; x < 12; x++){
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text(std::format("{}%%", slotTitles[x]).data());
					ImGui::SameLine();

					if(ImGui::BeginCombo(std::format("##pokeSlot{}Walk", x).data(), nullptr, ImGuiComboFlags_CustomPreview)){
						for(uint32_t i = 0; i < PokemonNames.size(); i++){
								bool is_selected = (mMapManager.mEncounters.mWalking[x] == i);
								ImGui::Image(mPokeIcons[i], {32,32});
								ImGui::SameLine();
								if (ImGui::Selectable(PokemonNames[i].data(), is_selected, ImGuiSelectableFlags_SpanAvailWidth, {-1, 32})){
									mMapManager.mEncounters.mWalking[x] = i;
								}
								if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					if(ImGui::BeginComboPreview()){
					    ImGui::SetCursorPosY(ImGui::GetCursorPosY()-10);
                        ImGui::Image(mPokeIcons[mMapManager.mEncounters.mWalking[x]], {32, 32});
                        ImGui::SameLine();
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY()+10);
                        ImGui::Text(PokemonNames[mMapManager.mEncounters.mWalking[x]].data());
                        ImGui::EndComboPreview();
					}


					ImGui::Text("Level");
					ImGui::SameLine();
					ImGui::InputInt(std::format("##pokeSlot{}WalkLevel", x).data(), (int*)&mMapManager.mEncounters.mWalkingLevel[x]);
					ImGui::NewLine();

					if(x < 5){
						ImGui::TableNextColumn();
						ImGui::Text(std::format("{}%%", fiveSlotTitles[x]).data());
						ImGui::SameLine();
						if(ImGui::BeginCombo(std::format("##pokeSlot{}Surf", x).data(), nullptr, ImGuiComboFlags_CustomPreview)){
							for(uint32_t i = 0; i < PokemonNames.size(); i++){
									bool is_selected = (mMapManager.mEncounters.mSurf[x] == i);
									ImGui::Image(mPokeIcons[i], {32,32});
									ImGui::SameLine();
									if (ImGui::Selectable(PokemonNames[i].data(), is_selected, ImGuiSelectableFlags_SpanAvailWidth, {-1, 32})){
										mMapManager.mEncounters.mSurf[x] = i;
									}
									if (is_selected) ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}
						if(ImGui::BeginComboPreview()){
						    ImGui::SetCursorPosY(ImGui::GetCursorPosY()-10);
                            ImGui::Image(mPokeIcons[mMapManager.mEncounters.mSurf[x]], {32, 32});
                            ImGui::SameLine();
                            ImGui::SetCursorPosY(   ImGui::GetCursorPosY()+10);
                            ImGui::Text(PokemonNames[mMapManager.mEncounters.mSurf[x]].data());
                            ImGui::EndComboPreview();
						}

						ImGui::Text("Max Level");
						ImGui::SameLine();
						ImGui::InputInt(std::format("##pokeSlot{}SurfMaxLevel", x).data(), (int*)&mMapManager.mEncounters.mSurfMaxLevels[x]);

						ImGui::Text("Min Level");
						ImGui::SameLine();
						ImGui::InputInt(std::format("##pokeSlot{}SurfMinLevel", x).data(), (int*)&mMapManager.mEncounters.mSurfMinLevels[x]);

						ImGui::TableNextColumn();
						ImGui::Text(std::format("{}%%", fiveSlotTitles[x]).data());
						ImGui::SameLine();
						if(ImGui::BeginCombo(std::format("##pokeSlot{}OldRod", x).data(), nullptr, ImGuiComboFlags_CustomPreview)){
							for(uint32_t i = 0; i < PokemonNames.size(); i++){
									bool is_selected = (mMapManager.mEncounters.mOldRod[x] == i);
									ImGui::Image(mPokeIcons[i], {32,32});
									ImGui::SameLine();
									if (ImGui::Selectable(PokemonNames[i].data(), is_selected, ImGuiSelectableFlags_SpanAvailWidth, {-1, 32})){
										mMapManager.mEncounters.mOldRod[x] = i;
									}
									if (is_selected) ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}
						if(ImGui::BeginComboPreview()){
						    ImGui::SetCursorPosY(ImGui::GetCursorPosY()-10);
                            ImGui::Image(mPokeIcons[mMapManager.mEncounters.mOldRod[x]], {32, 32});
                            ImGui::SameLine();
                            ImGui::SetCursorPosY(   ImGui::GetCursorPosY()+10);
                            ImGui::Text(PokemonNames[mMapManager.mEncounters.mOldRod[x]].data());
                            ImGui::EndComboPreview();
						}


						ImGui::Text("Max Level");
						ImGui::SameLine();
						ImGui::InputInt(std::format("##pokeSlot{}OldRodMaxLevel", x).data(), (int*)&mMapManager.mEncounters.mOldMaxLevels[x]);

						ImGui::Text("Min Level");
						ImGui::SameLine();
						ImGui::InputInt(std::format("##pokeSlot{}OldRodMinLevel", x).data(), (int*)&mMapManager.mEncounters.mOldMinLevels[x]);

						ImGui::TableNextColumn();
						ImGui::Text(std::format("{}%%", fiveSlotTitles[x]).data());
						ImGui::SameLine();
						if(ImGui::BeginCombo(std::format("##pokeSlot{}GoodRod", x).data(), nullptr, ImGuiComboFlags_CustomPreview)){
							for(uint32_t i = 0; i < PokemonNames.size(); i++){
									bool is_selected = (mMapManager.mEncounters.mGoodRod[x] == i);
									ImGui::Image(mPokeIcons[i], {32,32});
									ImGui::SameLine();
									if (ImGui::Selectable(PokemonNames[i].data(), is_selected, ImGuiSelectableFlags_SpanAvailWidth, {-1, 32})){
										mMapManager.mEncounters.mGoodRod[x] = i;
									}
									if (is_selected) ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}
						if(ImGui::BeginComboPreview()){
						    ImGui::SetCursorPosY(ImGui::GetCursorPosY()-10);
                            ImGui::Image(mPokeIcons[mMapManager.mEncounters.mGoodRod[x]], {32, 32});
                            ImGui::SameLine();
                            ImGui::SetCursorPosY(   ImGui::GetCursorPosY()+10);
                            ImGui::Text(PokemonNames[mMapManager.mEncounters.mGoodRod[x]].data());
                            ImGui::EndComboPreview();
						}

						ImGui::Text("Max Level");
						ImGui::SameLine();
						ImGui::InputInt(std::format("##pokeSlot{}GoodRodMaxLevel", x).data(), (int*)&mMapManager.mEncounters.mGoodRodMaxLevels[x]);

						ImGui::Text("Min Level");
						ImGui::SameLine();
						ImGui::InputInt(std::format("##pokeSlot{}GoodRodMinLevel", x).data(), (int*)&mMapManager.mEncounters.mGoodRodMinLevels[x]);

						ImGui::TableNextColumn();
						ImGui::Text(std::format("{}%%", fiveSlotTitles[x]).data());
						ImGui::SameLine();
						if(ImGui::BeginCombo(std::format("##pokeSlot{}SuperRod", x).data(), nullptr, ImGuiComboFlags_CustomPreview)){
							for(uint32_t i = 0; i < PokemonNames.size(); i++){
									bool is_selected = (mMapManager.mEncounters.mSuperRod[x] == i);
									ImGui::Image(mPokeIcons[i], {32,32});
									ImGui::SameLine();
									if (ImGui::Selectable(PokemonNames[i].data(), is_selected, ImGuiSelectableFlags_SpanAvailWidth, {-1, 32})){
										mMapManager.mEncounters.mSuperRod[x] = i;
									}
									if (is_selected) ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}
						if(ImGui::BeginComboPreview()){
						    ImGui::SetCursorPosY(ImGui::GetCursorPosY()-10);
                            ImGui::Image(mPokeIcons[mMapManager.mEncounters.mSuperRod[x]], {32, 32});
                            ImGui::SameLine();
                            ImGui::SetCursorPosY(   ImGui::GetCursorPosY()+10);
                            ImGui::Text(PokemonNames[mMapManager.mEncounters.mSuperRod[x]].data());
                            ImGui::EndComboPreview();
						}

						ImGui::Text("Max Level");
						ImGui::SameLine();
						ImGui::InputInt(std::format("##pokeSlot{}SuperRodMaxLevel", x).data(), (int*)&mMapManager.mEncounters.mSuperRodMaxLevels[x]);

						ImGui::Text("Min Level");
						ImGui::SameLine();
						ImGui::InputInt(std::format("##pokeSlot{}SuperRodMinLevel", x).data(), (int*)&mMapManager.mEncounters.mSuperRodMinLevels[x]);

					} else {
						for(int i = 0; i < 4; i++) ImGui::TableNextColumn();
					}
					//PokemonNames[mMapManager.mEncounters.mWalkingEncounters[x]].data()
				}

				ImGui::EndTable();

				ImGui::BeginTable("##timeEncounterData", 3, ImGuiTableFlags_Borders);
				ImGui::TableSetupColumn("Morning");
				ImGui::TableSetupColumn("Night");
				ImGui::TableSetupColumn("Swarm");
				ImGui::TableHeadersRow();
				ImGui::TableNextRow();

				ImGui::TableNextColumn();
				for(int x = 0; x < 2; x++){
					ImGui::Text("10%%");
					ImGui::SameLine();
					if(ImGui::BeginCombo(std::format("##pokeSlot{}Morning", x).data(), nullptr, ImGuiComboFlags_CustomPreview)){
						for(uint32_t i = 0; i < PokemonNames.size(); i++){
								bool is_selected = (mMapManager.mEncounters.mMorningPokemon[x] == i);
								ImGui::Image(mPokeIcons[i], {32,32});
								ImGui::SameLine();
								if (ImGui::Selectable(PokemonNames[i].data(), is_selected, ImGuiSelectableFlags_SpanAvailWidth, {-1, 32})){
									mMapManager.mEncounters.mMorningPokemon[x] = i;
								}
								if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					if(ImGui::BeginComboPreview()){
					    ImGui::SetCursorPosY(ImGui::GetCursorPosY()-10);
                        ImGui::Image(mPokeIcons[mMapManager.mEncounters.mMorningPokemon[x]], {32, 32});
                        ImGui::SameLine();
                        ImGui::SetCursorPosY(   ImGui::GetCursorPosY()+10);
                        ImGui::Text(PokemonNames[mMapManager.mEncounters.mMorningPokemon[x]].data());
                        ImGui::EndComboPreview();
					}
				}

				ImGui::TableNextColumn();
				for(int x = 0; x < 2; x++){
					ImGui::Text("10%%");
					ImGui::SameLine();
					if(ImGui::BeginCombo(std::format("##pokeSlot{}Night", x).data(), nullptr, ImGuiComboFlags_CustomPreview)){
						for(uint32_t i = 0; i < PokemonNames.size(); i++){
								bool is_selected = (mMapManager.mEncounters.mNightPokemon[x] == i);
								ImGui::Image(mPokeIcons[i], {32,32});
								ImGui::SameLine();
								if (ImGui::Selectable(PokemonNames[i].data(), is_selected, ImGuiSelectableFlags_SpanAvailWidth, {-1, 32})){
									mMapManager.mEncounters.mNightPokemon[x] = i;
								}
								if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					if(ImGui::BeginComboPreview()){
					    ImGui::SetCursorPosY(ImGui::GetCursorPosY()-10);
                        ImGui::Image(mPokeIcons[mMapManager.mEncounters.mNightPokemon[x]], {32, 32});
                        ImGui::SameLine();
                        ImGui::SetCursorPosY(   ImGui::GetCursorPosY()+10);
                        ImGui::Text(PokemonNames[mMapManager.mEncounters.mNightPokemon[x]].data());
                        ImGui::EndComboPreview();
					}
				}

				ImGui::TableNextColumn();
				for(int x = 0; x < 2; x++){
					ImGui::Text("20%%");
					ImGui::SameLine();
					if(ImGui::BeginCombo(std::format("##pokeSlot{}Swarm", x).data(), nullptr, ImGuiComboFlags_CustomPreview)){
						for(uint32_t i = 0; i < PokemonNames.size(); i++){
								bool is_selected = (mMapManager.mEncounters.mSwarmPokemon[x] == i);
								ImGui::Image(mPokeIcons[i], {32,32});
								ImGui::SameLine();
								if (ImGui::Selectable(PokemonNames[i].data(), is_selected, ImGuiSelectableFlags_SpanAvailWidth, {-1, 32})){
									mMapManager.mEncounters.mSwarmPokemon[x] = i;
								}
								if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					if(ImGui::BeginComboPreview()){
					    ImGui::SetCursorPosY(ImGui::GetCursorPosY()-10);
                        ImGui::Image(mPokeIcons[mMapManager.mEncounters.mSwarmPokemon[x]], {32, 32});
                        ImGui::SameLine();
                        ImGui::SetCursorPosY(   ImGui::GetCursorPosY()+10);
                        ImGui::Text(PokemonNames[mMapManager.mEncounters.mSwarmPokemon[x]].data());
                        ImGui::EndComboPreview();
					}
				}
				ImGui::EndTable();

				uint32_t textWidth = ImGui::CalcTextSize("Poke Radar").x;

				ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
				ImGui::Text("Poke Radar");

	//			ImGui::TableSetupColumn("Radar");
				ImGui::BeginTable("##radarEncounters", 4, ImGuiTableFlags_Borders);
				ImGui::TableSetupColumn("10%");
				ImGui::TableSetupColumn("10%");
				ImGui::TableSetupColumn("1%");
				ImGui::TableSetupColumn("1%");
				ImGui::TableHeadersRow();
				ImGui::TableNextRow();

				for(int x = 0; x < 4; x++){
					ImGui::TableNextColumn();
					if(ImGui::BeginCombo(std::format("##pokeSlotRadar{}", x).data(), nullptr, ImGuiComboFlags_CustomPreview)){
						for(uint32_t i = 0; i < PokemonNames.size(); i++){
								bool is_selected = (mMapManager.mEncounters.mRadarPokemon[x] == i);
								ImGui::Image(mPokeIcons[i], {32,32});
								ImGui::SameLine();
								if (ImGui::Selectable(PokemonNames[i].data(), is_selected, ImGuiSelectableFlags_SpanAvailWidth, {-1, 32})){
									mMapManager.mEncounters.mRadarPokemon[x] = i;
								}
								if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					if(ImGui::BeginComboPreview()){
					    ImGui::SetCursorPosY(ImGui::GetCursorPosY()-10);
                        ImGui::Image(mPokeIcons[mMapManager.mEncounters.mRadarPokemon[x]], {32, 32});
                        ImGui::SameLine();
                        ImGui::SetCursorPosY(   ImGui::GetCursorPosY()+10);
                        ImGui::Text(PokemonNames[mMapManager.mEncounters.mRadarPokemon[x]].data());
                        ImGui::EndComboPreview();
					}
				}

				ImGui::EndTable();

				textWidth = ImGui::CalcTextSize("Dual Cart Encounters").x;

				ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
				ImGui::Text("Dual Cart Encounters");


				ImGui::BeginTable("##encounterDataDualCart", 5, ImGuiTableFlags_Borders);
				ImGui::TableSetupColumn("Ruby");
				ImGui::TableSetupColumn("Sapphire");
				ImGui::TableSetupColumn("Emerald");
				ImGui::TableSetupColumn("Fire Red");
				ImGui::TableSetupColumn("Leaf Green");
				ImGui::TableHeadersRow();

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				for(int x = 0; x < 2; x++){
					ImGui::Text("4%%");
					ImGui::SameLine();
					if(ImGui::BeginCombo(std::format("##pokeSlot{}Ruby", x).data(), nullptr, ImGuiComboFlags_CustomPreview)){
						for(uint32_t i = 0; i < PokemonNames.size(); i++){
								bool is_selected = (mMapManager.mEncounters.mRuby[x] == i);
								ImGui::Image(mPokeIcons[i], {32,32});
								ImGui::SameLine();
								if (ImGui::Selectable(PokemonNames[i].data(), is_selected, ImGuiSelectableFlags_SpanAvailWidth, {-1, 32})){
									mMapManager.mEncounters.mRuby[x] = i;
								}
								if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					if(ImGui::BeginComboPreview()){
					    ImGui::SetCursorPosY(ImGui::GetCursorPosY()-10);
                        ImGui::Image(mPokeIcons[mMapManager.mEncounters.mRuby[x]], {32, 32});
                        ImGui::SameLine();
                        ImGui::SetCursorPosY(   ImGui::GetCursorPosY()+10);
                        ImGui::Text(PokemonNames[mMapManager.mEncounters.mRuby[x]].data());
                        ImGui::EndComboPreview();
					}
				}

				ImGui::TableNextColumn();
				for(int x = 0; x < 2; x++){
					ImGui::Text("4%%");
					ImGui::SameLine();
					if(ImGui::BeginCombo(std::format("##pokeSlot{}Sapphire", x).data(), nullptr, ImGuiComboFlags_CustomPreview)){
						for(uint32_t i = 0; i < PokemonNames.size(); i++){
								bool is_selected = (mMapManager.mEncounters.mSapphire[x] == i);
								ImGui::Image(mPokeIcons[i], {32,32});
								ImGui::SameLine();
								if (ImGui::Selectable(PokemonNames[i].data(), is_selected, ImGuiSelectableFlags_SpanAvailWidth, {-1, 32})){
									mMapManager.mEncounters.mSapphire[x] = i;
								}
								if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					if(ImGui::BeginComboPreview()){
					    ImGui::SetCursorPosY(ImGui::GetCursorPosY()-10);
                        ImGui::Image(mPokeIcons[mMapManager.mEncounters.mSapphire[x]], {32, 32});
                        ImGui::SameLine();
                        ImGui::SetCursorPosY(   ImGui::GetCursorPosY()+10);
                        ImGui::Text(PokemonNames[mMapManager.mEncounters.mSapphire[x]].data());
                        ImGui::EndComboPreview();
					}
				}

				ImGui::TableNextColumn();
				for(int x = 0; x < 2; x++){
					ImGui::Text("4%%");
					ImGui::SameLine();
					if(ImGui::BeginCombo(std::format("##pokeSlot{}Emerald", x).data(), nullptr, ImGuiComboFlags_CustomPreview)){
						for(uint32_t i = 0; i < PokemonNames.size(); i++){
								bool is_selected = (mMapManager.mEncounters.mEmerald[x] == i);
								ImGui::Image(mPokeIcons[i], {32,32});
								ImGui::SameLine();
								if (ImGui::Selectable(PokemonNames[i].data(), is_selected, ImGuiSelectableFlags_SpanAvailWidth, {-1, 32})){
									mMapManager.mEncounters.mEmerald[x] = i;
								}
								if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					if(ImGui::BeginComboPreview()){
					    ImGui::SetCursorPosY(ImGui::GetCursorPosY()-10);
                        ImGui::Image(mPokeIcons[mMapManager.mEncounters.mEmerald[x]], {32, 32});
                        ImGui::SameLine();
                        ImGui::SetCursorPosY(   ImGui::GetCursorPosY()+10);
                        ImGui::Text(PokemonNames[mMapManager.mEncounters.mEmerald[x]].data());
                        ImGui::EndComboPreview();
					}
				}

				ImGui::TableNextColumn();
				for(int x = 0; x < 2; x++){
					ImGui::Text("4%%");
					ImGui::SameLine();
					if(ImGui::BeginCombo(std::format("##pokeSlot{}FireRed", x).data(), nullptr, ImGuiComboFlags_CustomPreview)){
						for(uint32_t i = 0; i < PokemonNames.size(); i++){
								bool is_selected = (mMapManager.mEncounters.mFireRed[x] == i);
								ImGui::Image(mPokeIcons[i], {32,32});
								ImGui::SameLine();
								if (ImGui::Selectable(PokemonNames[i].data(), is_selected, ImGuiSelectableFlags_SpanAvailWidth, {-1, 32})){
									mMapManager.mEncounters.mFireRed[x] = i;
								}
								if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					if(ImGui::BeginComboPreview()){
					    ImGui::SetCursorPosY(ImGui::GetCursorPosY()-10);
                        ImGui::Image(mPokeIcons[mMapManager.mEncounters.mFireRed[x]], {32, 32});
                        ImGui::SameLine();
                        ImGui::SetCursorPosY(   ImGui::GetCursorPosY()+10);
                        ImGui::Text(PokemonNames[mMapManager.mEncounters.mFireRed[x]].data());
                        ImGui::EndComboPreview();
					}
				}

				ImGui::TableNextColumn();
				for(int x = 0; x < 2; x++){
					ImGui::Text("4%%");
					ImGui::SameLine();
					if(ImGui::BeginCombo(std::format("##pokeSlot{}LeafGreen", x).data(), nullptr, ImGuiComboFlags_CustomPreview)){
						for(uint32_t i = 0; i < PokemonNames.size(); i++){
								bool is_selected = (mMapManager.mEncounters.mLeafGreen[x] == i);
								ImGui::Image(mPokeIcons[i], {32,32});
								ImGui::SameLine();
								if (ImGui::Selectable(PokemonNames[i].data(), is_selected, ImGuiSelectableFlags_SpanAvailWidth, {-1, 32})){
									mMapManager.mEncounters.mLeafGreen[x] = i;
								}
								if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					if(ImGui::BeginComboPreview()){
					    ImGui::SetCursorPosY(ImGui::GetCursorPosY()-10);
                        ImGui::Image(mPokeIcons[mMapManager.mEncounters.mLeafGreen[x]], {32, 32});
                        ImGui::SameLine();
                        ImGui::SetCursorPosY(   ImGui::GetCursorPosY()+10);
                        ImGui::Text(PokemonNames[mMapManager.mEncounters.mLeafGreen[x]].data());
                        ImGui::EndComboPreview();
					}
				}

				ImGui::EndTable();
			}
			ImGui::PopStyleVar();
		}


	ImGui::End();
	ImGui::PopStyleVar();

	if(mPermsTooltip){
		ImGui::SetNextWindowPos(ImVec2(mPermsTooltipX - 25, mPermsTooltipY - 22.5 + scroll));
		ImGui::SetNextWindowSize(ImVec2(100, 65));
		ImGui::Begin("##mPermsTooltip", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

		char buf[3] = {};
		sprintf(buf, "%x", mSelectedChunkPtr->GetMovementPermissions()[(mSelectedY * 32) + mSelectedX].first);
		ImGui::PushItemWidth(-1.0f);
		if(ImGui::InputText("##hex_input", buf, sizeof(buf), ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsHexadecimal)){
			mSelectedChunkPtr->GetMovementPermissions()[(mSelectedY * 32) + mSelectedX].first = strtol(buf, NULL, 16);
		}
		ImGui::PopItemWidth();
		if(ImGui::Button("Close", ImVec2(-1, 0))) mPermsTooltip = false;
		ImGui::End();
	}

	//mGrid.Render(mCamera.GetPosition(), mCamera.GetProjectionMatrix(), mCamera.GetViewMatrix());
}

void URotomContext::RenderMainWindow(float deltaTime) {


}

void URotomContext::RenderMenuBar() {
	mOptionsOpen = false;
	ImGui::BeginMainMenuBar();

	if (ImGui::BeginMenu("File")) {
		if (ImGui::MenuItem(ICON_FK_FOLDER_OPEN " Open...")) {
			bIsFileDialogOpen = true;
		}
		if (ImGui::MenuItem(ICON_FK_FLOPPY_O " Save...")) {
			bIsSaveDialogOpen = true;
		}

		ImGui::Separator();
		ImGui::MenuItem(ICON_FK_WINDOW_CLOSE " Close");

		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Edit")) {
		if(ImGui::MenuItem(ICON_FK_COG " Settings")){
			mOptionsOpen = true;
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu(ICON_FK_QUESTION_CIRCLE)) {
		if(ImGui::MenuItem("About")){
			mAboutOpen = true;
		}
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();

	if (bIsFileDialogOpen) {
		IGFD::FileDialogConfig config;
		ImGuiFileDialog::Instance()->OpenDialog("OpenRomDialog", "Choose Pokemon ROM", ".nds", config);
	}

	if (ImGuiFileDialog::Instance()->Display("OpenRomDialog")) {
		if (ImGuiFileDialog::Instance()->IsOk()) {
			std::string FilePath = ImGuiFileDialog::Instance()->GetFilePathName();
			std::cout << FilePath << std::endl;

			//try {
				// load rom here
				mRom = std::make_unique<Palkia::Nitro::Rom>(std::filesystem::path(FilePath));

				// convert stuff
				auto areaWinArc = mRom->GetFile("arc/area_win_gra.narc");
				bStream::CMemoryStream areaWinStrm(areaWinArc->GetData(), areaWinArc->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);

				auto areaWin = Palkia::Nitro::Archive(areaWinStrm);
				for(int i = 0; i < 9; i++){
                    auto ncgrFile = areaWin.GetFileByIndex(i * 2);
                    auto nclrFile = areaWin.GetFileByIndex((i * 2) + 1);

                    Palkia::Formats::NCGR ncgr;
                    Palkia::Formats::NCLR nclr;

                    bStream::CMemoryStream ncgrStrm(ncgrFile->GetData(), ncgrFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
                    bStream::CMemoryStream nclrStrm(nclrFile->GetData(), nclrFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);

                    ncgr.Load(ncgrStrm);
                    nclr.Load(nclrStrm);

                    auto data = ncgr.Convert(136, 40, nclr);

    				glGenTextures(1, &mAreaTypeImages[i]);
                    glBindTexture(GL_TEXTURE_2D, mAreaTypeImages[i]);

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 136, 40, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
                    glBindTexture(GL_TEXTURE_2D, 0);

				}

				// load poke icons, there are. a lot of them! this could be a bit slow.
                glGenTextures(mPokeIcons.size(), &mPokeIcons[0]);
                auto pokeIconArcFile = mRom->GetFile("poketool/icongra/pl_poke_icon.narc");
				bStream::CMemoryStream pokeIconStrm(pokeIconArcFile->GetData(), pokeIconArcFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
                auto pokeIconArc = Palkia::Nitro::Archive(pokeIconStrm);

                Palkia::Formats::NCLR pokePalette;
                auto pPalFile = pokeIconArc.GetFileByIndex(0);
                bStream::CMemoryStream pPalStream(pPalFile->GetData(), pPalFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
                pokePalette.Load(pPalStream);
				for(int i = 0; i < mPokeIcons.size(); i++){
                    auto ncgrFile = pokeIconArc.GetFileByIndex(i+7);

                    Palkia::Formats::NCGR ncgr;
                    bStream::CMemoryStream ncgrStrm(ncgrFile->GetData(), ncgrFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);

                    ncgr.Load(ncgrStrm);

                    auto data = ncgr.Convert(32, 32, pokePalette);

                    glBindTexture(GL_TEXTURE_2D, mPokeIcons[i]);

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
                    glBindTexture(GL_TEXTURE_2D, 0);

				}

				// Load Area Names
				std::cout << "0x" << std::hex << mRom->GetHeader().gameCode << std::dec << " : " << Configs[mRom->GetHeader().gameCode].mMsgPath << std::endl;
				auto msgArchive = mRom->GetFile(Configs[mRom->GetHeader().gameCode].mMsgPath);
				bStream::CMemoryStream msgArcStream(msgArchive->GetData(), msgArchive->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);

				auto msgs = Palkia::Nitro::Archive(msgArcStream);

				auto locationNamesFile = msgs.GetFileByIndex(Configs[mRom->GetHeader().gameCode].mLocationNamesFileID);
				bStream::CMemoryStream locationNamesStream(locationNamesFile->GetData(), locationNamesFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);

				mLocationNames = Text::DecodeStringList(locationNamesStream);

				auto pokemonNamesFile = msgs.GetFileByIndex(Configs[mRom->GetHeader().gameCode].mPokeNamesFileID);
				LoadPokemonNames(pokemonNamesFile);

				mLocationNames.shrink_to_fit();

				mMapManager.Init(mRom.get(), mLocationNames);

				auto mmodelArchive = mRom->GetFile(Configs[mRom->GetHeader().gameCode].mMoveModel);
				bStream::CMemoryStream mmodelArchiveStream(mmodelArchive->GetData(), mmodelArchive->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);

				auto mmodels = Palkia::Nitro::Archive(mmodelArchiveStream);
				LoadEventModel(mmodels, mPointRenderer);

				// Load sprite ID list for overworld evs
				auto overlay = mRom->GetOverlays9()[5].file.lock();
				// table offest 0x2BC34
				bStream::CMemoryStream owTableStream(overlay->GetData(), overlay->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
				owTableStream.seek(0x2BC34); // todo: load this from game config
				uint32_t entryID = 0;
				while(entryID != 0xFFFF && owTableStream.tell() + 8 < owTableStream.tell()){
					entryID = owTableStream.readUInt32();
				    uint32_t spriteID = owTableStream.readUInt32();
				    mOverworldSpriteIDs[entryID] = spriteID;
				}

			//}
			//catch (std::runtime_error e) {
			//	std::cout << "Failed to load rom " << FilePath << "! Exception: " << e.what() << "\n";
			//}
			//catch (std::exception e) {
			//	std::cout << "Failed to load rom " << FilePath << "! Exception: " << e.what() << "\n";
			//}

			bIsFileDialogOpen = false;
		} else {
			bIsFileDialogOpen = false;
		}

		ImGuiFileDialog::Instance()->Close();
	}

	if (bIsSaveDialogOpen) {
		IGFD::FileDialogConfig config;
		ImGuiFileDialog::Instance()->OpenDialog("SaveRomDialog", "Save Pokemon ROM", ".nds", config);
	}

	if (ImGuiFileDialog::Instance()->Display("SaveRomDialog")) {
		if (ImGuiFileDialog::Instance()->IsOk()) {
			std::string FilePath = ImGuiFileDialog::Instance()->GetFilePathName();
			std::cout << FilePath << std::endl;

			try {
				mMapManager.Save(mRom.get());

				// save rom here
				mRom->Save(FilePath);

			}
			catch (std::runtime_error e) {
				std::cout << "Failed to save rom " << FilePath << "! Exception: " << e.what() << "\n";
			}
			catch (std::exception e) {
				std::cout << "Failed to save rom " << FilePath << "! Exception: " << e.what() << "\n";
			}

			bIsSaveDialogOpen = false;
		} else {
			bIsSaveDialogOpen = false;
		}

		ImGuiFileDialog::Instance()->Close();
	}

	if (ImGui::BeginPopupModal("ROM Load Error", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)){
		ImGui::Text("Error Loading Rom!\n\n");
		ImGui::Separator();

		if (ImGui::Button("OK", ImVec2(120,0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if(mAboutOpen){
		ImGui::OpenPopup("About Window");
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowSize(ImVec2(250, 130), ImGuiCond_Always);
		ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.4f));
	}

	if (ImGui::BeginPopupModal("About Window", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)){
		auto windowWidth = ImGui::GetWindowSize().x;
		auto textWidth = ImGui::CalcTextSize("Rotom").x;
		ImGuiStyle* style = &ImGui::GetStyle();

		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::Text("Rotom");

		ImGui::Separator();

		textWidth = ImGui::CalcTextSize("https://github.com/Astral-C/Rotom").x;

		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::Text("https://github.com/Astral-C/Rotom");

		textWidth = ImGui::CalcTextSize("Made by veebs").x;

		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::Text("Made by veebs");

		ImGui::Separator();

		float size = 120 + style->FramePadding.x * 2.0f;
		float avail = ImGui::GetContentRegionAvail().x;

		float off = (avail - size) * 0.5;
		if (off > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

		if (ImGui::Button("Close", ImVec2(120, 0))) {
			mAboutOpen = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if(mOptionsOpen){
		ImGui::OpenPopup("Options");
	}

}

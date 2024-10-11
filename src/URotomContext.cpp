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
#include "Util.hpp"

URotomContext::~URotomContext(){
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glDeleteFramebuffers(1, &mFbo);
	glDeleteRenderbuffers(1, &mRbo);
	glDeleteTextures(1, &mViewTex);
	glDeleteTextures(1, &mPickTex);
}

URotomContext::URotomContext(){	
	srand(time(0));

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
		ImGui::Text("Current Chunk");
		ImGui::Separator();
		
		// Show Chunk Settings
		// Buildings, npcs, etc
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

			for(auto sprite : mMapManager.mEvents.overworldEvents){
				RenderEvent(projection * view * glm::translate(glm::mat4(1.0f), glm::vec3(((sprite.x)*16)-256, Palkia::fixed(sprite.z), ((sprite.y+1)*16)-256)));
			}

			cursorPos = ImGui::GetCursorScreenPos();
			ImGui::Image(reinterpret_cast<void*>(static_cast<uintptr_t>(mViewTex)), { winSize.x, winSize.y }, {0.0f, 1.0f}, {1.0f, 0.0f});

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
					if(mMapManager.GetActiveMatrix() != nullptr){
						auto result = mMapManager.GetActiveMatrix()->Select(id);
						if(result.first != nullptr){
							mSelectedBuilding = result.first;
							mSelectedChunk.x = result.second.first;
							mSelectedChunk.y = result.second.second;
						}
					}
				} else {
					mSelectedBuilding = nullptr;
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
				
			}

			// gizmo operation on selected
			//if(ImGuizmo::Manipulate(&mCamera.GetViewMatrix()[0][0], &mCamera.GetProjectionMatrix()[0][0], (ImGuizmo::OPERATION)mGizmoOperation, ImGuizmo::WORLD, &transform[0][0], &delta[0][0])){
			//		object->mTransform = glm::inverse(zoneTransform) * transform;
			//}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		float scroll = 0.0f;
		if(mCurrentTool == "Movement Permissions" && mRom != nullptr){
			ImVec2 padding = ImGui::GetStyle().CellPadding;
			ImGui::GetStyle().CellPadding = ImVec2(0.0f, 0.0f);
			ImGui::BeginChild("##movementPositions");
			scroll = ImGui::GetScrollMaxY() - ImGui::GetScrollY();
			if(mSelectedChunkPtr == nullptr && mMapManager.GetActiveMatrix() != nullptr){
				auto entries = mMapManager.GetActiveMatrix()->GetEntries();

				if(mMapManager.GetActiveMatrix()->GetHeight() == 1 && mMapManager.GetActiveMatrix()->GetWidth() == 1){
					mSelectedChunkPtr = entries[0].mChunk.lock();
				}
				
				ImGui::BeginTable("##mapMatrixView", mMapManager.GetActiveMatrix()->GetWidth(), ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX);

				for(std::size_t y = 0; y < mMapManager.GetActiveMatrix()->GetHeight(); y++){
					for(std::size_t x = 0; x < mMapManager.GetActiveMatrix()->GetWidth(); x++){
						if(!entries[(y * mMapManager.GetActiveMatrix()->GetWidth()) + x].mChunk.expired()){
							bool isInLocation = mLocationNames[entries[(y * mMapManager.GetActiveMatrix()->GetWidth()) + x].mChunkHeader.lock()->mPlaceNameID] == mCurrentLocation;
							
							if(isInLocation) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4, 0.8, 0.5, 1.0));
							if(ImGui::Button(std::format("{}, {}", x, y).c_str(), ImVec2(-1.0,0))){
								mSelectedChunkPtr = entries[(y * mMapManager.GetActiveMatrix()->GetWidth()) + x].mChunk.lock();
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

		if(mCurrentTool == "Encounter Editor" && mRom != nullptr){
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {5, 5});
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
				if(ImGui::BeginCombo(std::format("##pokeSlot{}Walk", x).data(), PokemonNames[mMapManager.mEncounters.mWalking[x]].data())){
					for(uint32_t i = 0; i < PokemonNames.size(); i++){
							bool is_selected = (mMapManager.mEncounters.mWalking[x] == i);
							if (ImGui::Selectable(PokemonNames[i].data(), is_selected)){
								mMapManager.mEncounters.mWalking[x] = i;
							}
							if (is_selected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				
				ImGui::Text("Level");
				ImGui::SameLine();
				ImGui::InputInt(std::format("##pokeSlot{}WalkLevel", x).data(), (int*)&mMapManager.mEncounters.mWalkingLevel[x]);
				ImGui::NewLine();

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

			auto windowWidth = ImGui::GetWindowSize().x;
			auto textWidth = ImGui::CalcTextSize("Time Based").x;

			ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
			ImGui::Text("Time Based");

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
			}

			ImGui::TableNextColumn();
			for(int x = 0; x < 2; x++){
				ImGui::Text("10%%");
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
			}

			ImGui::TableNextColumn();
			for(int x = 0; x < 2; x++){
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
			ImGui::EndTable();

			textWidth = ImGui::CalcTextSize("Poke Radar").x;

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
				if(ImGui::BeginCombo(std::format("##pokeSlotRadar{}", x).data(), PokemonNames[mMapManager.mEncounters.mRadarPokemon[x]].data())){
					for(uint32_t i = 0; i < PokemonNames.size(); i++){
							bool is_selected = (mMapManager.mEncounters.mRadarPokemon[x] == i);
							if (ImGui::Selectable(PokemonNames[i].data(), is_selected)){
								mMapManager.mEncounters.mRadarPokemon[x] = i;
							}
							if (is_selected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
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
				if(ImGui::BeginCombo(std::format("##pokeSlot{}Ruby", x).data(), PokemonNames[mMapManager.mEncounters.mRuby[x]].data())){
					for(uint32_t i = 0; i < PokemonNames.size(); i++){
							bool is_selected = (mMapManager.mEncounters.mRuby[x] == i);
							if (ImGui::Selectable(PokemonNames[i].data(), is_selected)){
								mMapManager.mEncounters.mRuby[x] = i;
							}
							if (is_selected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}

			ImGui::TableNextColumn();
			for(int x = 0; x < 2; x++){
				ImGui::Text("4%%");
				ImGui::SameLine();
				if(ImGui::BeginCombo(std::format("##pokeSlot{}Sapphire", x).data(), PokemonNames[mMapManager.mEncounters.mSapphire[x]].data())){
					for(uint32_t i = 0; i < PokemonNames.size(); i++){
							bool is_selected = (mMapManager.mEncounters.mSapphire[x] == i);
							if (ImGui::Selectable(PokemonNames[i].data(), is_selected)){
								mMapManager.mEncounters.mSapphire[x] = i;
							}
							if (is_selected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}

			ImGui::TableNextColumn();
			for(int x = 0; x < 2; x++){
				ImGui::Text("4%%");
				ImGui::SameLine();
				if(ImGui::BeginCombo(std::format("##pokeSlot{}Emerald", x).data(), PokemonNames[mMapManager.mEncounters.mEmerald[x]].data())){
					for(uint32_t i = 0; i < PokemonNames.size(); i++){
							bool is_selected = (mMapManager.mEncounters.mEmerald[x] == i);
							if (ImGui::Selectable(PokemonNames[i].data(), is_selected)){
								mMapManager.mEncounters.mEmerald[x] = i;
							}
							if (is_selected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}

			ImGui::TableNextColumn();
			for(int x = 0; x < 2; x++){
				ImGui::Text("4%%");
				ImGui::SameLine();
				if(ImGui::BeginCombo(std::format("##pokeSlot{}FireRed", x).data(), PokemonNames[mMapManager.mEncounters.mFireRed[x]].data())){
					for(uint32_t i = 0; i < PokemonNames.size(); i++){
							bool is_selected = (mMapManager.mEncounters.mFireRed[x] == i);
							if (ImGui::Selectable(PokemonNames[i].data(), is_selected)){
								mMapManager.mEncounters.mFireRed[x] = i;
							}
							if (is_selected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}

			ImGui::TableNextColumn();
			for(int x = 0; x < 2; x++){
				ImGui::Text("4%%");
				ImGui::SameLine();
				if(ImGui::BeginCombo(std::format("##pokeSlot{}LeafGreen", x).data(), PokemonNames[mMapManager.mEncounters.mLeafGreen[x]].data())){
					for(uint32_t i = 0; i < PokemonNames.size(); i++){
							bool is_selected = (mMapManager.mEncounters.mLeafGreen[x] == i);
							if (ImGui::Selectable(PokemonNames[i].data(), is_selected)){
								mMapManager.mEncounters.mLeafGreen[x] = i;
							}
							if (is_selected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}

			ImGui::EndTable();
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

			try {
				// load rom here
				mRom = std::make_unique<Palkia::Nitro::Rom>(std::filesystem::path(FilePath));

				// Load Area Names

				auto msgArchive = mRom->GetFile("msgdata/pl_msg.narc");
				bStream::CMemoryStream msgArcStream(msgArchive->GetData(), msgArchive->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);

				auto msgs = Palkia::Nitro::Archive(msgArcStream);

				auto locationNamesFile = msgs.GetFileByIndex(433);
				bStream::CMemoryStream locationNamesStream(locationNamesFile->GetData(), locationNamesFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);

				mLocationNames = Text::DecodeStringList(locationNamesStream);

				auto pokemonNamesFile = msgs.GetFileByIndex(412);
				LoadPokemonNames(pokemonNamesFile);
				mLocationNames.shrink_to_fit();
				
				mMapManager.Init(mRom.get(), mLocationNames);

				auto mmodelArchive = mRom->GetFile("data/mmodel/mmodel.narc");
				bStream::CMemoryStream mmodelArchiveStream(mmodelArchive->GetData(), mmodelArchive->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);

				auto mmodels = Palkia::Nitro::Archive(mmodelArchiveStream);
				LoadEventModel(mmodels.GetFileByIndex(421));
			}
			catch (std::runtime_error e) {
				std::cout << "Failed to load rom " << FilePath << "! Exception: " << e.what() << "\n";
			}
			catch (std::exception e) {
				std::cout << "Failed to load rom " << FilePath << "! Exception: " << e.what() << "\n";
			}

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

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
#include <algorithm>

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
		for(auto zone : mLocationNames){
			if(mCurrentLocation == zone){
				ImGui::TextColored(ImVec4(0.5, 1.0, 0.5, 1.0), zone.data());
			} else {
				ImGui::Text(zone.data());
				if(ImGui::IsItemClicked(0)){
					mCurrentLocation = zone;
					mMapManager.LoadZone(std::find(mLocationNames.begin(), mLocationNames.end(), zone) - mLocationNames.begin());
				}
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
					//Switch tool
					ImGui::EndTabItem();
				}
			}
		ImGui::EndTabBar();

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
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, (uint32_t)winSize.x, (uint32_t)winSize.y, 0, GL_RED_INTEGER, GL_INT, nullptr);
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
			glReadPixels(static_cast<GLint>(pickPos.x), static_cast<GLint>(pickPos.y), 1, 1, GL_RED_INTEGER, GL_INT, (void*)&id);

			if(id != 0){
				// selection made on nsbmds!!
			}

			ImGuizmo::BeginFrame();
			ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
			ImGuizmo::SetRect(cursorPos.x, cursorPos.y, winSize.x, winSize.y);

			// gizmo operation on selected
			//if(ImGuizmo::Manipulate(&mCamera.GetViewMatrix()[0][0], &mCamera.GetProjectionMatrix()[0][0], (ImGuizmo::OPERATION)mGizmoOperation, ImGuizmo::WORLD, &transform[0][0], &delta[0][0])){
			//		object->mTransform = glm::inverse(zoneTransform) * transform;
			//}

			// [veebs]: Fix this, imguizmo update broke it

		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	ImGui::End();
	ImGui::PopStyleVar();

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

				
				auto locationNamesArc = Palkia::Nitro::Archive(msgArcStream);
				auto locationNamesFile = locationNamesArc.GetFileByIndex(433);
				bStream::CMemoryStream locationNamesStream(locationNamesFile->GetData(), locationNamesFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);

				mLocationNames = Text::DecodeStringList(locationNamesStream);

				mMapManager.Init(mRom.get());

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

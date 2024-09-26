#pragma once

#include "UApplication.hpp"

class URotomApplication : public UApplication {
	struct GLFWwindow* mWindow;
	class URotomContext* mContext;

	virtual bool Execute(float deltaTime) override;

	
public:

	URotomApplication();
	virtual ~URotomApplication() {}

	virtual bool Setup() override;
	virtual bool Teardown() override;
};

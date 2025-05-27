#pragma once
#include <EASTL/vector.h>
#include <qdIce/qdBase/base.h>
#include <qdIce/qdCore/nodeBase.h>


FORWARD_DECLARATION_2(qd, Application);

FORWARD_DECLARATION_2(qd, UiView);
FORWARD_DECLARATION_3(qd, action, Action);


namespace qd {
namespace ui {
	
	class Desktop : public qd::Node {
	    eastl::vector<ref_ptr<UiView>> windows;
	    qd::Application* mApp = nullptr;
	
	public:
	    virtual ~Desktop();
	
	    void drawImGuiMainFrame();
	
	    void _drawMainToolBar();
	    void _drawDebuggerWindows();
	    void destroy();
	
	    template <class T>
	    inline T* getWnd_() const {
	        const uint32_t idx = T::CLASS_ID;
	        UiView* curView = windows[idx];
	        return static_cast<T*>(curView);
	    }
	
	    void addView(UiView* view);
	
	private:
	    void _drawMainMenuBar();
	
	};  // class GUIManager
	
	
}; // namespace ui

};  // namespace qd

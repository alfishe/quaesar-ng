#pragma once
#include <EASTL/vector.h>
#include <qd/Base/base.h>
#include <qd/Core/nodeBase.h>
#include <qd/TypeSystem/typeDeclare.h>


FORWARD_DECLARATION_2(qd, Application);
FORWARD_DECLARATION_2(qd, UiView);
FORWARD_DECLARATION_3(qd, operation, Operation);


namespace qd {

	class UiDesktop : public qd::Node {
        TS_REFLECT_CLASS(qd::UiDesktop, qd::Node);

	    eastl::vector<ref_ptr<UiView>> windows;
	    qd::Application* mApp = nullptr;

	public:
	    virtual ~UiDesktop();

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

	};  // class UiDesktop


};  // namespace qd

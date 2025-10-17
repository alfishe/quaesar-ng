#pragma once
#include "qd/base/baseTypes.h"
#include "qd/enum/enumToString.h"
#include "qd/stl/string.h"
#include "qd/typeSystem/typeDeclare.h"


struct CfgBase {
    virtual ~CfgBase() = default;
};


#define CFG_DECLARE(TCfgClass)            \
    TS_REFLECT_CLASS(TCfgClass, CfgBase); \
    static TCfgClass& get()               \
    {                                     \
        static TCfgClass instance;        \
        return instance;                  \
    }



struct EVmModel {
    enum EType {
        UNDEF,
        A500,
        A1200,
        END,
    };
    ENUM_DECLARE_BASE(::, EVmModel, EType, UNDEF);
};

struct EVmModelCfg {
    enum EType {
        UNDEF,
        Chip512Kb = 1 << 10,
        Chip1Mb = 1 << 11,
        Chip2Mb = 1 << 12,
        Chip4Mb = 1 << 13,

        Fast512Kb = 1 << 20,
        Fast1Mb = 1 << 21,
        Fast2Mb = 1 << 22,
        Fast4Mb = 1 << 23,

        A500_DEF = EVmModelCfg::Chip512Kb | EVmModelCfg::Fast512Kb,
        END,
    };
    ENUM_DECLARE_BASE(::, EVmModelCfg, EType, UNDEF);
    ENUM_DECLARE_FLAGS()
};  // struct EQuaeModelCfg
//////////////////////////////////////////////////////////////////////////


//------------------------------------------------------------------------
struct CfgVmPrefs : public CfgBase {
    CFG_DECLARE(CfgVmPrefs);
    EVmModel model = EVmModel::A500;
    EVmModelCfg modelCfg = EVmModelCfg::A500_DEF;
};
inline static CfgVmPrefs& g_cfg_vm_prefs = CfgVmPrefs::get();

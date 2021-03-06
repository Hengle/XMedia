#include "entry/entry.h"
#include "imgui/imgui.h"
#include "XImage.hpp"
#include "XLayer.hpp"
#include "XFilterEffectListUI.hpp"
#include "XFrameOutput.hpp"
#include "XMixtureUI.hpp"
#include "XTransform.hpp"
#include "camera.h"
#include "bx/timer.h"

USING_NS_X_IMAGE
class XImageExample : public entry::AppI {
public:
    XImageExample(const char* _name, const char* _description)
            : entry::AppI(_name, _description) {}

    void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override {
        // 初始化
        Args args(_argc, _argv);
        mWidth  = _width;
        mHeight = _height;
        mDebug = BGFX_DEBUG_TEXT;
        mReset = BGFX_RESET_VSYNC;
        bgfx::Init init;
        init.type     = args.m_type;
        init.vendorId = args.m_pciId;
        init.resolution.width  = mWidth;
        init.resolution.height = mHeight;
        init.resolution.reset  = BGFX_RESET_VSYNC;
        XImage::init(init);

        mHorizontalMargin = 10;
        mVerticalMargin = 10;
        mMenuWidth = mWidth / 4;
        float ratio = 1920.0f / 1080.0f;
        mLayerWidth = (mWidth - mMenuWidth - 3 * mHorizontalMargin) / 2;
        mLayerHeight = (mHeight - 3 * mVerticalMargin) / 2;
        if (mLayerWidth / mLayerHeight > ratio) {
            mLayerWidth = mLayerHeight * ratio;
        } else if (mLayerWidth / mLayerHeight < ratio) {
            mLayerHeight = mLayerWidth / ratio;
        }
        mHorizontalMargin = (mWidth - 2 * mLayerWidth - mMenuWidth) / 3;
        mVerticalMargin = (mHeight - 2 * mLayerHeight) / 3;

        // 创建调试ui
        imguiCreate();

        // 初始化图层与默认滤镜
        mFilterEffectListUIs.push_back(new XFilterEffectListUI());
        mFilterEffectListUIs.push_back(new XFilterEffectListUI());
        mFilterEffectListUIs.push_back(new XFilterEffectListUI());
        mFilterEffectListUIs.push_back(new XFilterEffectListUI());
        mFilterEffectListUIs.push_back(new XFilterEffectListUI());
        mBlendUIs.push_back(new XMixtureUI());
        mBlendUIs.push_back(new XMixtureUI());
        mBlendUIs.push_back(new XMixtureUI());
        mBlendUIs.push_back(new XMixtureUI());
        mBlendUIs.push_back(new XMixtureUI());

        XFrameOutput *spring = new XFrameOutput();
        spring->setPath("images/spring.jpg");
        mFrameOutputs.push_back(spring);
        XFrameOutput *summer = new XFrameOutput();
        summer->setPath("images/summer.jpg");
        mFrameOutputs.push_back(summer);
        XFrameOutput *autumn = new XFrameOutput();
        autumn->setPath("images/autumn.jpg");
        mFrameOutputs.push_back(autumn);
        XFrameOutput *winter = new XFrameOutput();
        winter->setPath("images/winter.jpg");
        mFrameOutputs.push_back(winter);
        XFrameOutput *leaves = new XFrameOutput();
        leaves->setPath("images/leaves.jpg");
        mFrameOutputs.push_back(leaves);


        mLayers.push_back(new XLayer(0));
        XRect leftTop = {static_cast<int>(mHorizontalMargin), static_cast<int>(mVerticalMargin),
                         static_cast<unsigned int>(mLayerWidth), static_cast<unsigned int>(mLayerHeight)};
        spring->setOutputSize(leftTop.width, leftTop.height);
        mLayers[0]->setViewRect(leftTop);
        mLayers[0]->setSource(spring);

        mLayers.push_back(new XLayer(1));
        XRect rightTop = {static_cast<int>(mLayerWidth + mHorizontalMargin * 2), static_cast<int>(mVerticalMargin),
                          static_cast<unsigned int>(mLayerWidth), static_cast<unsigned int>(mLayerHeight)};
        summer->setOutputSize(rightTop.width, rightTop.height);
        mLayers[1]->setViewRect(rightTop);
        mLayers[1]->setSource(summer);

        mLayers.push_back(new XLayer(2));
        XRect leftBottom = {static_cast<int>(mHorizontalMargin), static_cast<int>(mLayerHeight + mVerticalMargin * 2),
                            static_cast<unsigned int>(mLayerWidth), static_cast<unsigned int>(mLayerHeight)};
        autumn->setOutputSize(leftBottom.width, leftBottom.height);
        mLayers[2]->setViewRect(leftBottom);
        mLayers[2]->setSource(autumn);

        mLayers.push_back(new XLayer(3));
        XRect rightBottom = {static_cast<int>(mLayerWidth + mHorizontalMargin * 2),
                             static_cast<int>(mLayerHeight + mVerticalMargin * 2),
                             static_cast<unsigned int>(mLayerWidth), static_cast<unsigned int>(mLayerHeight)};
        winter->setOutputSize(rightBottom.width, rightBottom.height);
        mLayers[3]->setViewRect(rightBottom);
        mLayers[3]->setSource(winter);

        mLayers.push_back(new XLayer(4));
        float centerWidth = mLayerWidth * 1.3f;
        float centerHeight = centerWidth / ratio;
        XRect center = {static_cast<int>((mWidth - mMenuWidth) / 2 - centerWidth / 2),
                             static_cast<int>(mHeight / 2 - centerHeight / 2),
                             static_cast<unsigned int>(centerWidth), static_cast<unsigned int>(centerHeight)};
        leaves->setOutputSize(center.width, center.height);
        mLayers[4]->setViewRect(center);
        mLayers[4]->setSource(leaves);

        // 图层内遮罩临时Demo代码
//        XFrameLayer *matte = new XFrameLayer(6);
//        matte->setPath("images/winter.jpg");
//        matte->setViewRect(center);
//        matte->setMixer(new XMixer(XMixerType::ADD));
//        mLayers[4]->addMask(matte);

        XImage::addLayer(mLayers[0]);
        XImage::addLayer(mLayers[1]);
        XImage::addLayer(mLayers[2]);
        XImage::addLayer(mLayers[3]);
        XImage::addLayer(mLayers[4]);

        mTransformEffect = new XTransform();
        XImage::addGlobalEffect(mTransformEffect);

        cameraCreate();
        cameraSetPosition({ 0.0f, 0.0f, -1.0f });
        cameraSetVerticalAngle(0.0f);
        cameraSetHorizontalAngle(0.0f);

        m_timeOffset = bx::getHPCounter();
    }

    virtual int shutdown() override {
        for (XLayer *layer : mLayers) {
            SAFE_DELETE(layer);
        }
        mLayers.clear();
        for (XFrameOutput *frameOutput : mFrameOutputs) {
            SAFE_DELETE(frameOutput);
        }
        mFrameOutputs.clear();
        for (XFilterEffectListUI *filterUi : mFilterEffectListUIs) {
            SAFE_DELETE(filterUi);
        }
        mFilterEffectListUIs.clear();
        for (XMixtureUI *blendUi : mBlendUIs) {
            SAFE_DELETE(blendUi);
        }
        mBlendUIs.clear();

        SAFE_DELETE(mTransformEffect);

        imguiDestroy();

        cameraDestroy();

        XImage::shutdown();

        return 0;
    }

    bool update() override {
        if (!entry::processEvents(mWidth, mHeight, mDebug, mReset, &mMouseState)) {
            // 刷新调试界面
            imguiBeginFrame(mMouseState.m_mx
                            ,  mMouseState.m_my
                            , (mMouseState.m_buttons[entry::MouseButton::Left  ] ? IMGUI_MBUT_LEFT   : 0)
                            | (mMouseState.m_buttons[entry::MouseButton::Right ] ? IMGUI_MBUT_RIGHT  : 0)
                            | (mMouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
                            ,  mMouseState.m_mz
                            , uint16_t(mWidth)
                            , uint16_t(mHeight)
                            );
            ImGui::SetNextWindowPos(
                    ImVec2(mHorizontalMargin * 3 + mLayerWidth * 2, 0)
                    , ImGuiCond_FirstUseEver
            );
            ImGui::SetNextWindowSize(
                    ImVec2(mMenuWidth, mHeight)
                    , ImGuiCond_FirstUseEver
            );
            ImGui::Begin("Settings"
                    , NULL
            );

            ImGui::Text("Layer:");
            for (uint32_t ii = 0; ii < mLayers.size(); ++ii) {
                ImGui::SameLine();
                char name[16];
                bx::snprintf(name, BX_COUNTOF(name), "%d", ii);
                ImGui::RadioButton(name, &mCurrentLayer, ii);
            }
            mFilterEffectListUIs[mCurrentLayer]->imgui(mLayers[mCurrentLayer]);
            mBlendUIs[mCurrentLayer]->imgui(mLayers[mCurrentLayer]);
            if (mIsFirstRenderCall) {
                mIsFirstRenderCall = false;
                mFilterEffectListUIs[0]->imgui(mLayers[0]);
                mFilterEffectListUIs[1]->imgui(mLayers[1]);
                mFilterEffectListUIs[2]->imgui(mLayers[2]);
                mFilterEffectListUIs[3]->imgui(mLayers[3]);
                mFilterEffectListUIs[4]->imgui(mLayers[4]);
            }

            ImGui::Separator();
            ImGui::Text("Global Effects:");

            transform();

            ImGui::End();
            imguiEndFrame();

            // 刷新图层及效果
            XImage::begin();
            XImage::submit();
            XImage::frame();
            XImage::end();

            return true;
        }
        
        return false;
    }

    void cameraReset() {
        cameraDestroy();
        cameraCreate();
        cameraSetPosition({ 0.0f, 0.0f, -1.0f });
        cameraSetVerticalAngle(0.0f);
        cameraSetHorizontalAngle(0.0f);
    }

    void transform() {
        ImGui::Separator();
        ImGui::Text("Transform:");
        const char *PROJECTION_TYPE[] = {"Orthogonal", "Perspective"};
        int size = BX_COUNTOF(PROJECTION_TYPE);
        ImGui::Combo("Projection Type", &mCurrentProjection, PROJECTION_TYPE, size, size);
        if (mLastProjection != mCurrentProjection || ImGui::Button("Reset")) {
            cameraReset();
        }

        int64_t now = bx::getHPCounter() - m_timeOffset;
        static int64_t last = now;
        const int64_t frameTime = now - last;
        last = now;
        const double freq = double(bx::getHPFrequency());
        const float deltaTime = float(frameTime / freq);
        cameraUpdate(deltaTime / 15.0f, mMouseState);
        mTransformEffect->setCamera(cameraGetPosition(), cameraGetAt(), {0.0f, 1.0f, 0.0f});
        XEffectProcessor *transform = dynamic_cast<XEffectProcessor *>(mTransformEffect->get());
        if (PROJECTION_TYPE[mCurrentProjection] == "Orthogonal") {
            mTransformEffect->setOrthogonal(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 100.0f);
        } else if (PROJECTION_TYPE[mCurrentProjection] == "Perspective") {
            mTransformEffect->setPerspective(60.0f, static_cast<float>(mWidth) / static_cast<float>(mHeight), 0.1f, 100.0f);
        }

        mLastProjection = mCurrentProjection;
    }

private:
    entry::MouseState mMouseState;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mLayerWidth;
    uint32_t mLayerHeight;
    uint32_t mMenuWidth;
    uint32_t mHorizontalMargin;
    uint32_t mVerticalMargin;
    uint32_t mDebug;
    uint32_t mReset;

    std::vector<XLayer *> mLayers;
    std::vector<XFrameOutput *> mFrameOutputs;
    std::vector<XFilterEffectListUI *> mFilterEffectListUIs;
    std::vector<XMixtureUI *> mBlendUIs;

    XTransform *mTransformEffect;
    int mCurrentProjection = 0;
    int mLastProjection = 0;

    int mCurrentLayer = 0;
    bool mIsFirstRenderCall = true;
    int64_t m_timeOffset;
};


ENTRY_IMPLEMENT_MAIN(XImageExample, "X-Image", "X-Image's default example");

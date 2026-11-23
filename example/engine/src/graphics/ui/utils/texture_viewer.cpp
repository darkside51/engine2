#include "texture_viewer.h"

#include <imgui.h>

namespace engine {
    
    void TextureViewer::setTexture(std::string_view name, const rhi::runtime::ImagePtr & image) {
        if (_images.empty()) {
            const auto & [n, _] = _images.emplace(std::string{name}, image);
            _currentName = n->first;
        }
        _images[name] = image;
    }

    void TextureViewer::removeTexture(std::string_view name) {
        _images.erase(std::string{name});
    }

    void TextureViewer::show(bool &showed) {
        ImGui::SetNextWindowSize(ImVec2(128, 128), ImGuiCond_FirstUseEver);
        if(!ImGui::Begin("texture viewer", &showed, 0)) {
            ImGui::End();
            return;
        }

        if (_images.empty()) {
            ImGui::End();
            return;
        }

        if (ImGui::BeginCombo("texture", _currentName.data(), 0)) {
            size_t i = 0u;
            for (auto & [name, img] : _images) {
                const bool selected = (_currentImageIdx == i);

                if (ImGui::Selectable(name.c_str(), selected)) {
                    _currentImageIdx = i;
                    _currentName = name;
                    _texture = &img;
                }

                // set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        const ImVec2 viewerSize = ImGui::GetContentRegionAvail();
        if (_texture) {
            ImGui::Image(_texture, viewerSize);
        }

        ImGui::End();
    }

    void TextureViewer::destroy() {
        _images.clear();
    }
}
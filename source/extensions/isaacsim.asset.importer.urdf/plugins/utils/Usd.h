// SPDX-FileCopyrightText: Copyright (c) 2024-2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

/**
 * Select a existing layer as edit target.
 *
 * @param stage The stage of the operation.
 * @param layerIdentifier Layer identifier.
 * @return true if the layer is selected, false otherwise.
 *
 **/
static bool setAuthoringLayer(pxr::UsdStageRefPtr stage, const std::string& layerIdentifier)
{
    const auto& sublayer = pxr::SdfLayer::Find(layerIdentifier);
    if (!sublayer || !stage->HasLocalLayer(sublayer))
    {
        return false;
    }

    pxr::UsdEditTarget editTarget = stage->GetEditTargetForLocalLayer(sublayer);
    stage->SetEditTarget(editTarget);

    return true;
}

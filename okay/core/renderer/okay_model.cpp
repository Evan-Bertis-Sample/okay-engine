#include "okay_model.hpp"

using namespace okay;

OkayModel OkayModelBuffer::AddModel(const std::vector<OkayVertex>& model) {
    OkayModel m;
    m.Offset = Positions.size();
    m.Length = model.size();

    Positions.reserve(Positions.size() + model.size());
    Normals.reserve(Normals.size() + model.size());
    UVs.reserve(UVs.size() + model.size());
    Colors.reserve(Colors.size() + model.size());

    for (const OkayVertex& v : model) {
        Positions.push_back(v.Position);
        Normals.push_back(v.Normal);
        UVs.push_back(v.UV);
        Colors.push_back(v.Color);
    }

    return m;
}

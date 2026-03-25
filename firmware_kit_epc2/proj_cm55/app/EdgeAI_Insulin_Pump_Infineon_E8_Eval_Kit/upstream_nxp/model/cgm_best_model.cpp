#include "cgm_best_model.hpp"

#include <cmath>

namespace cgm_best {

constexpr float kMedian[] = {1.41231162e+09f, 153f, 0f, 0f, 0f, 153f, 153f, 153f, 0f, 0f, 0f, 0f, 0f, 0f, 0f, 0f, 0f, 0f, 0f, 0f, 153f, 152.166667f};
constexpr float kMean[] = {1.41231437e+09f, 161.571851f, 0.019511744f, 0.0107268356f, 0.0302385796f, 161.511929f, 161.578879f, 161.572961f, 0.0293138524f, 0.0209913076f, 0.0277418162f, 0.0185870168f, 0.0135934899f, 0.0170149806f, 0.0107268356f, 0.00739781764f, 0.0107268356f, 0.102274829f, 0.065840577f, 0.0438320695f, 161.517046f, 161.464324f};
constexpr float kScale[] = {107195.478f, 72.7958856f, 0.296453206f, 0.462257358f, 0.595325169f, 72.7898528f, 72.7863758f, 72.7967991f, 0.594594315f, 0.512262699f, 0.584593319f, 0.294949161f, 0.235648109f, 0.274168252f, 0.462257358f, 0.392198775f, 0.462257358f, 8.37204126f, 12.7560555f, 8.70697817f, 72.589269f, 72.2452693f};
constexpr float kCoeff[] = {0.488943967f, 46.2656272f, -0.592405709f, 0.608197949f, 0.177253391f, 23.2966538f, 12.4313354f, -11.2912215f, -0.16703515f, -0.0382644838f, -0.120659613f, -0.0652513372f, 0.347428845f, 0.756264129f, -0.173220225f, -0.258727014f, -0.601137902f, -1.44728973f, 9.78859574f, 3.8914332f, 46.2718828f, -47.3280462f};
constexpr float kIntercept = 161.723876f;

bool IsClassification()
{
    return false;
}

float PredictScore(const float *features, std::size_t len)
{
    if ((features == nullptr) || (len < kFeatureCount))
    {
        return 0.0f;
    }

    float acc = kIntercept;
    for (std::size_t i = 0; i < kFeatureCount; ++i)
    {
        float v = features[i];
        if (!std::isfinite(v))
        {
            v = kMedian[i];
        }
        v = (v - kMean[i]) / kScale[i];
        acc += (kCoeff[i] * v);
    }

    return acc;
}

} // namespace cgm_best

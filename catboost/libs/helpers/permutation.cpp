#include "permutation.h"

template<typename TDataType>
static inline void ApplyPermutation(const TVector<ui64>& permutation, TVector<TDataType>* elements) {
    TVector<ui64> toIndices(permutation);
    const ui64 elementCount = elements->size();
    for (ui64 elementIdx = 0; elementIdx < elementCount; ++elementIdx) {
        while (toIndices[elementIdx] != elementIdx) {
            auto destinationIndex = toIndices[elementIdx];
            DoSwap((*elements)[elementIdx], (*elements)[destinationIndex]);
            DoSwap(toIndices[elementIdx], toIndices[destinationIndex]);
        }
    }
}

void ApplyPermutation(const TVector<ui64>& permutation, TPool* pool, NPar::TLocalExecutor* localExecutor) {
    Y_VERIFY(pool->Docs.GetDocCount() == 0 || permutation.size() == pool->Docs.GetDocCount());

    if (pool->Docs.GetDocCount() > 0) {
        NPar::TLocalExecutor::TExecRangeParams blockParams(0, pool->Docs.Factors.ysize());
        localExecutor->ExecRange([&] (int factorIdx) {
            ApplyPermutation(permutation, &pool->Docs.Factors[factorIdx]);
        }, blockParams, NPar::TLocalExecutor::WAIT_COMPLETE);

        for (int dim = 0; dim < pool->Docs.GetBaselineDimension(); ++dim) {
            ApplyPermutation(permutation, &pool->Docs.Baseline[dim]);
        }
        ApplyPermutation(permutation, &pool->Docs.Target);
        ApplyPermutation(permutation, &pool->Docs.Weight);
        ApplyPermutation(permutation, &pool->Docs.Id);
        ApplyPermutation(permutation, &pool->Docs.GroupId);
        ApplyPermutation(permutation, &pool->Docs.QueryId);
    }

    for (auto& pair : pool->Pairs) {
        pair.WinnerId = permutation[pair.WinnerId];
        pair.LoserId = permutation[pair.LoserId];
    }
}

TVector<ui64> InvertPermutation(const TVector<ui64>& permutation) {
    TVector<ui64> result(permutation.size());
    for (ui64 i = 0; i < permutation.size(); ++i) {
        result[permutation[i]] = i;
    }
    return result;
}

TVector<ui64> CreateOrderByKey(const TVector<ui64>& key) {
    TVector<ui64> indices(key.size());
    std::iota(indices.begin(), indices.end(), 0);

    std::sort(
        indices.begin(),
        indices.end(),
        [&key](ui64 i1, ui64 i2) {
            return key[i1] < key[i2];
        }
    );

    return indices;
}

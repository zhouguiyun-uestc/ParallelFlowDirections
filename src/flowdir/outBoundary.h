#pragma once
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <vector>

class OutBoundary {
private:
    friend class cereal::access;
    template < class Archive > void serialize( Archive& ar ) {
        ar( dem, low, high, ID );
    }

public:
    std::vector< float > dem;
    std::vector< int > low, high;
    std::vector< int > ID;

public:
    OutBoundary() = default;
    OutBoundary( std::vector< float > dem, std::vector< int > low, std::vector< int > high, std::vector< int > ID );
    ~OutBoundary() = default;
};

#include <string>

#include "functions/filter_function.h"
#include "functions/map_function.h"
#include "functions/sink_function.h"
#include "functions/source_function.h"
#include "gtest/gtest.h"

namespace extream {
namespace {

struct BadMap {
    auto operator()(Event<int>&) -> int { return 42; }
};

struct GoodMap {
    auto operator()(Event<int>& e) -> Event<std::string> {
        return Event<std::string>(std::to_string(e.payload()), e.timestamp());
    }
};

TEST(MapFunctionConcept, SatisfiedByCorrectLambda) {
    auto lambda = [](Event<int>& e) -> Event<std::string> {
        return Event<std::string>(std::to_string(e.payload()), e.timestamp());
    };
    static_assert(MapFunction<decltype(lambda), int, std::string>);
}

TEST(MapFunctionConcept, SatisfiedByCorrectFunctor) {
    static_assert(MapFunction<GoodMap, int, std::string>);
}

TEST(MapFunctionConcept, RejectedByWrongReturnType) {
    static_assert(!MapFunction<BadMap, int, std::string>);
}

struct BadFilter {
    auto operator()(Event<int>&) -> int { return 0; }
};

struct GoodFilter {
    auto operator()(Event<int>& e) -> bool { return e.payload() > 0; }
};

TEST(FilterFunctionConcept, SatisfiedByCorrectLambda) {
    auto lambda = [](Event<int>& e) -> bool { return e.payload() > 0; };
    static_assert(FilterFunction<decltype(lambda), int>);
}

TEST(FilterFunctionConcept, SatisfiedByCorrectFunctor) {
    static_assert(FilterFunction<GoodFilter, int>);
}

TEST(FilterFunctionConcept, RejectedByWrongReturnType) {
    static_assert(!FilterFunction<BadFilter, int>);
}

struct BadSink {
    auto operator()(Event<int>&) -> int { return 0; }
};

struct GoodSink {
    void operator()(Event<int>&) {}
};

TEST(SinkFunctionConcept, SatisfiedByCorrectLambda) {
    auto lambda = [](Event<int>&) -> void {};
    static_assert(SinkFunction<decltype(lambda), int>);
}

TEST(SinkFunctionConcept, SatisfiedByCorrectFunctor) {
    static_assert(SinkFunction<GoodSink, int>);
}

TEST(SinkFunctionConcept, RejectedByWrongReturnType) {
    static_assert(!SinkFunction<BadSink, int>);
}

struct BadSource {
    auto operator()(SourceEmitter<int>&) -> int { return 0; }
};

struct GoodSource {
    void operator()(SourceEmitter<int>& emitter) { emitter.emit(Event<int>(42, i64(1000))); }
};

TEST(SourceFunctionConcept, SatisfiedByCorrectLambda) {
    auto lambda = [](SourceEmitter<int>& emitter) -> void { emitter.emit(Event<int>(1, i64(0))); };
    static_assert(SourceFunction<decltype(lambda), int>);
}

TEST(SourceFunctionConcept, SatisfiedByCorrectFunctor) {
    static_assert(SourceFunction<GoodSource, int>);
}

TEST(SourceFunctionConcept, RejectedByWrongReturnType) {
    static_assert(!SourceFunction<BadSource, int>);
}

}  // namespace
}  // namespace extream

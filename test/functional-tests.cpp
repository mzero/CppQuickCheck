/*
 * Copyright (c) 2019, Philipp Claßen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "catch.hpp"
#include "cppqc.h"

using namespace cppqc;

namespace FunctionalTestsFixtures {

struct MinimalPassingProperty : cppqc::Property<bool> {
  bool check(const bool& /*v*/) const override { return true; }
  std::string name() const override { return "Dummy check (will always pass)"; }
};

struct MinimalFailingProperty : cppqc::Property<bool> {
  bool check(const bool& /*v*/) const override {
    return false;  // fails intentionally
  }
  std::string name() const override { return "Dummy check (will always fail)"; }
};

struct NontriviallyFailingProperty : cppqc::Property<std::vector<int>> {
  bool check(const std::vector<int>& v) const override {
    if (v.size() >= 1 && (v[0] % 99) == 1) {
      return false;
    }

    return true;
  }
};

struct MinimalPassingIntRefProperty : cppqc::Property<int> {
  bool check(const int& /*v*/) const override { return true; }
  std::string name() const override {
    return "int& dummy check (will always pass)";
  }
};

struct ComplicatedNestedTypeWithHardToFindFailingProperty
    : cppqc::Property<std::vector<
          std::pair<std::vector<std::string>, std::pair<size_t, char>>>> {
  bool check(const std::vector<
             std::pair<std::vector<std::string>, std::pair<size_t, char>>>& v)
      const override {
    if (v.empty()) {
      return true;
    }

    for (const auto& v_ : v) {
      const std::vector<std::string>& v1 = v_.first;
      const std::pair<size_t, char>& p2 = v_.second;
      if (v1.empty()) {
        continue;
      }

      const std::string& last = v1[v1.size() - 1];
      if (last.empty()) {
        continue;
      }

      if (last.size() == p2.first && last[0] == p2.second) {
        // counter-example found!
        return false;
      }
    }
    return true;
  }
};

}  // namespace FunctionalTestsFixtures

TEST_CASE("minimal passing example (bool)", "[functional]") {
  const Result result =
      quickCheckOutput(FunctionalTestsFixtures::MinimalPassingProperty{});

  REQUIRE(result.result == QC_SUCCESS);
}

TEST_CASE("minimal passing example (int)", "[functional]") {
  const Result result =
      quickCheckOutput(FunctionalTestsFixtures::MinimalPassingIntRefProperty{});

  REQUIRE(result.result == QC_SUCCESS);
}

TEST_CASE("minimal failing example", "[functional]") {
  const Result result =
      quickCheckOutput(FunctionalTestsFixtures::MinimalFailingProperty{});

  REQUIRE(result.result == QC_FAILURE);
}

TEST_CASE("tests with fixed seeds must be repeatible", "[functional][seed]") {
  for (int i = 0; i < 100; i++) {
    const auto seed = static_cast<SeedType>(i);

    std::ostringstream output1;
    const Result run1 =
        quickCheckOutput(FunctionalTestsFixtures::NontriviallyFailingProperty{},
                         output1, 100, 0, 0, DISABLE_SHRINK_TIMEOUT, seed);
    REQUIRE(run1.seed == seed);

    std::ostringstream output2;
    const Result run2 =
        quickCheckOutput(FunctionalTestsFixtures::NontriviallyFailingProperty{},
                         output2, 100, 0, 0, DISABLE_SHRINK_TIMEOUT, seed);
    REQUIRE(run2.seed == seed);

    REQUIRE(run1.result == run2.result);
    REQUIRE(output1.str() == output2.str());
  }
}

TEST_CASE("tests with nested types and a non-trivial failure", "[functional]") {
  // a counter-example should be found after a few thousand samples,
  // but in this test rather be too conservative with the limit
  constexpr std::size_t maxSuccess = 1000000;
  detail::NullOstream out;
  const Result result =
      quickCheckOutput(FunctionalTestsFixtures::
                           ComplicatedNestedTypeWithHardToFindFailingProperty{},
                       out, maxSuccess);

  REQUIRE(result.result == QC_FAILURE);
}

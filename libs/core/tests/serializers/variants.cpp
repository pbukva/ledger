//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/serializers/group_definitions.hpp"
#include "core/serializers/main_serializer.hpp"

#include "variant/variant.hpp"

#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace {

using fetch::variant::Variant;
using fetch::serializers::MsgPackSerializer;

TEST(MsgPacker, VariantInt)
{

  {
    MsgPackSerializer stream;
    Variant           i1, i2;

    i1     = 123456;
    stream = MsgPackSerializer();
    stream << i1;
    stream.seek(0);
    stream >> i2;
    EXPECT_EQ(i2.As<int>(), 123456);
  }
}

TEST(MsgPacker, VariantFloat)
{
  {
    MsgPackSerializer stream;
    Variant           fp1, fp2;

    fp1    = 1.25;  // should be repn exactly.
    stream = MsgPackSerializer();
    stream << fp1;
    stream.seek(0);
    stream >> fp2;
    EXPECT_EQ(fp2.As<float>(), 1.25);
  }
}

TEST(MsgPacker, VariantString)
{
  {
    MsgPackSerializer stream;
    Variant           s1, s2;

    s1     = "123456";
    stream = MsgPackSerializer();
    stream << s1;
    stream.seek(0);
    stream >> s2;
    EXPECT_EQ(s2.As<std::string>(), "123456");
  }
}

TEST(MsgPacker, VariantNull)
{
  {
    MsgPackSerializer stream;
    Variant           null1, null2;

    null1  = Variant::Null();
    stream = MsgPackSerializer();
    stream << null1;
    stream.seek(0);
    stream >> null2;
    EXPECT_EQ(null2.IsNull(), true);
  }
}

TEST(MsgPacker, VariantArray)
{
  {
    MsgPackSerializer stream;
    Variant           arr1, arr2;

    arr1    = Variant::Array(4);
    arr1[0] = Variant::Null();
    arr1[1] = 123456;
    arr1[2] = 1.25;
    arr1[3] = "123456";
    stream  = MsgPackSerializer();
    stream << arr1;
    stream.seek(0);
    stream >> arr2;
    EXPECT_EQ(arr2.IsArray(), true);
    EXPECT_EQ(arr2[0].IsNull(), true);
    EXPECT_EQ(arr2[1].As<int>(), 123456);
    EXPECT_EQ(arr2[2].As<float>(), 1.25);
    EXPECT_EQ(arr2[3].As<std::string>(), "123456");
  }
}

TEST(MsgPacker, VariantArrayOfArray)
{

  {
    MsgPackSerializer stream;
    Variant           arr1, arr2;

    arr1    = Variant::Array(4);
    arr1[0] = Variant::Array(4);
    arr1[1] = Variant::Array(4);
    arr1[2] = Variant::Array(4);
    arr1[3] = Variant::Array(4);

    for (std::size_t i = 0; i < 4; i++)
    {
      for (std::size_t j = 0; j < 4; j++)
      {
        arr1[i][j] = (i == j) ? 1 : 0;
      }
    }

    stream = MsgPackSerializer();
    stream << arr1;
    stream.seek(0);
    stream >> arr2;

    EXPECT_EQ(arr2.IsArray(), true);
    for (std::size_t i = 0; i < 4; i++)
    {
      EXPECT_EQ(arr2[i].IsArray(), true);
      for (std::size_t j = 0; j < 4; j++)
      {
        EXPECT_EQ(arr2[i][j].As<int>(), (i == j) ? 1 : 0);
      }
    }
  }
}

TEST(MsgPacker, VariantObject)
{
  {
    MsgPackSerializer stream;
    Variant           obj1, obj2;

    obj1 = Variant::Object();

    obj1["foo"] = 1;
    obj1["bar"] = 2;

    stream = MsgPackSerializer();
    stream << obj1;
    stream.seek(0);
    stream >> obj2;

    EXPECT_EQ(obj2.IsObject(), true);
    EXPECT_EQ(obj2["foo"].As<int>(), 1);
    EXPECT_EQ(obj2["bar"].As<int>(), 2);
  }
}

constexpr std::size_t VariantTypeSize()
{
  using Type = Variant::Type;
  std::size_t    size{0};
  constexpr auto first = Type::UNDEFINED;
  switch (first)
  {
  case first:
    ++size;
    /* FALLTHRU */
  case Type::INTEGER:
    ++size;
    /* FALLTHRU */
  case Type::FLOATING_POINT:
    ++size;
    /* FALLTHRU */
  case Type::FIXED_POINT:
    ++size;
    /* FALLTHRU */
  case Type::BOOLEAN:
    ++size;
    /* FALLTHRU */
  case Type::STRING:
    ++size;
    /* FALLTHRU */
  case Type::NULL_VALUE:
    ++size;
    /* FALLTHRU */
  case Type::ARRAY:
    ++size;
    /* FALLTHRU */
  case Type::OBJECT:
    ++size;
    /* FALLTHRU */
  }
  return size;
}

constexpr Variant::Type IndexToVariantType(std::size_t i)
{
  using Type = Variant::Type;
  static_assert(VariantTypeSize() == static_cast<std::size_t>(Type::OBJECT) + 1ull,
                "Something is wrong with conversion from integer to Variant::Type enumeration");
  return static_cast<Type>(i % VariantTypeSize());
}

Variant CreateVariant(std::size_t remaining_nest_level, Variant::Type type = Variant::Type::OBJECT)
{
  using Type = Variant::Type;

  type = remaining_nest_level > 0 ? type : Type::STRING;
  auto const index{static_cast<std::size_t>(type)};

  switch (type)
  {
  case Type::UNDEFINED:
    return Variant::Undefined();
  case Type::NULL_VALUE:
    return Variant::Null();
  case Type::INTEGER:
    return Variant{index};
  case Type::FLOATING_POINT:
    return Variant{static_cast<double>(index)};
  case Type::FIXED_POINT:
    return Variant{Variant::FixedPointRepr(index)};
  case Type::BOOLEAN:
    return Variant{index % 2 > 0};
  case Type::STRING:
    return Variant{std::to_string(index)};
  case Type::ARRAY:
  {
    auto arr{Variant::Array(VariantTypeSize())};
    for (std::size_t i{0}; i < arr.size(); ++i)
    {
      arr[i] = CreateVariant(remaining_nest_level - 1, IndexToVariantType(i));
    }
    return arr;
  }
  case Type::OBJECT:
  {
    auto obj{Variant::Object()};
    for (std::size_t i{0}; i < VariantTypeSize(); ++i)
    {
      obj[std::to_string(i)] = CreateVariant(remaining_nest_level - 1, IndexToVariantType(i));
    }
    return obj;
  }
  }

  return Variant{};
}

TEST(MsgPacker, ComplexVariant)
{
  {
    MsgPackSerializer stream;
    Variant           obj1, obj2;

    obj1 = CreateVariant(6ull, Variant::Type::OBJECT);

    stream = MsgPackSerializer();
    stream << obj1;
    stream.seek(0);
    stream >> obj2;

    EXPECT_EQ(obj2.IsObject(), true);
    EXPECT_EQ(obj1, obj2);
    obj2["@qwerty@"] = 3.14;
    EXPECT_NE(obj1, obj2);
  }
}

}  // namespace

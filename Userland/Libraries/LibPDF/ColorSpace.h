/*
 * Copyright (c) 2021-2022, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/DeprecatedFlyString.h>
#include <AK/Forward.h>
#include <LibGfx/Color.h>
#include <LibGfx/ICC/Profile.h>
#include <LibPDF/Value.h>

#define ENUMERATE_COLOR_SPACE_FAMILIES(V) \
    V(DeviceGray, true)                   \
    V(DeviceRGB, true)                    \
    V(DeviceCMYK, true)                   \
    V(CalGray, false)                     \
    V(CalRGB, false)                      \
    V(Lab, false)                         \
    V(ICCBased, false)                    \
    V(Indexed, false)                     \
    V(Pattern, true)                      \
    V(Separation, false)                  \
    V(DeviceN, false)

namespace PDF {

class ColorSpaceFamily {
public:
    ColorSpaceFamily(DeprecatedFlyString name, bool may_be_specified_directly)
        : m_name(move(name))
        , m_may_be_specified_directly(may_be_specified_directly)
    {
    }

    DeprecatedFlyString name() const { return m_name; }
    bool may_be_specified_directly() const { return m_may_be_specified_directly; }
    static PDFErrorOr<ColorSpaceFamily> get(DeprecatedFlyString const&);

#define ENUMERATE(name, may_be_specified_directly) static ColorSpaceFamily name;
    ENUMERATE_COLOR_SPACE_FAMILIES(ENUMERATE)
#undef ENUMERATE

private:
    DeprecatedFlyString m_name;
    bool m_may_be_specified_directly;
};

class ColorSpace : public RefCounted<ColorSpace> {
public:
    static PDFErrorOr<NonnullRefPtr<ColorSpace>> create(DeprecatedFlyString const&);
    static PDFErrorOr<NonnullRefPtr<ColorSpace>> create(Document*, NonnullRefPtr<ArrayObject>);

    virtual ~ColorSpace() = default;

    virtual PDFErrorOr<Color> color(ReadonlySpan<Value> arguments) const = 0;
    virtual int number_of_components() const = 0;
    virtual Vector<float> default_decode() const = 0;
    virtual ColorSpaceFamily const& family() const = 0;
};

class DeviceGrayColorSpace final : public ColorSpace {
public:
    static NonnullRefPtr<DeviceGrayColorSpace> the();

    ~DeviceGrayColorSpace() override = default;

    PDFErrorOr<Color> color(ReadonlySpan<Value> arguments) const override;
    int number_of_components() const override { return 1; }
    Vector<float> default_decode() const override;
    ColorSpaceFamily const& family() const override { return ColorSpaceFamily::DeviceGray; }

private:
    DeviceGrayColorSpace() = default;
};

class DeviceRGBColorSpace final : public ColorSpace {
public:
    static NonnullRefPtr<DeviceRGBColorSpace> the();

    ~DeviceRGBColorSpace() override = default;

    PDFErrorOr<Color> color(ReadonlySpan<Value> arguments) const override;
    int number_of_components() const override { return 3; }
    Vector<float> default_decode() const override;
    ColorSpaceFamily const& family() const override { return ColorSpaceFamily::DeviceRGB; }

private:
    DeviceRGBColorSpace() = default;
};

class DeviceCMYKColorSpace final : public ColorSpace {
public:
    static NonnullRefPtr<DeviceCMYKColorSpace> the();

    ~DeviceCMYKColorSpace() override = default;

    PDFErrorOr<Color> color(ReadonlySpan<Value> arguments) const override;
    int number_of_components() const override { return 4; }
    Vector<float> default_decode() const override;
    ColorSpaceFamily const& family() const override { return ColorSpaceFamily::DeviceCMYK; }

private:
    DeviceCMYKColorSpace() = default;
};

class CalRGBColorSpace final : public ColorSpace {
public:
    static PDFErrorOr<NonnullRefPtr<CalRGBColorSpace>> create(Document*, Vector<Value>&& parameters);

    ~CalRGBColorSpace() override = default;

    PDFErrorOr<Color> color(ReadonlySpan<Value> arguments) const override;
    int number_of_components() const override { return 3; }
    Vector<float> default_decode() const override;
    ColorSpaceFamily const& family() const override { return ColorSpaceFamily::CalRGB; }

private:
    CalRGBColorSpace() = default;

    Array<float, 3> m_whitepoint { 0, 0, 0 };
    Array<float, 3> m_blackpoint { 0, 0, 0 };
    Array<float, 3> m_gamma { 1, 1, 1 };
    Array<float, 9> m_matrix { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
};

class ICCBasedColorSpace final : public ColorSpace {
public:
    static PDFErrorOr<NonnullRefPtr<ColorSpace>> create(Document*, Vector<Value>&& parameters);

    ~ICCBasedColorSpace() override = default;

    PDFErrorOr<Color> color(ReadonlySpan<Value> arguments) const override;
    int number_of_components() const override;
    Vector<float> default_decode() const override;
    ColorSpaceFamily const& family() const override { return ColorSpaceFamily::ICCBased; }

private:
    ICCBasedColorSpace(NonnullRefPtr<Gfx::ICC::Profile>);

    static RefPtr<Gfx::ICC::Profile> s_srgb_profile;
    NonnullRefPtr<Gfx::ICC::Profile> m_profile;
};

class SeparationColorSpace final : public ColorSpace {
public:
    static PDFErrorOr<NonnullRefPtr<SeparationColorSpace>> create(Document*, Vector<Value>&& parameters);

    ~SeparationColorSpace() override = default;

    PDFErrorOr<Color> color(ReadonlySpan<Value> arguments) const override;
    int number_of_components() const override { return 1; }
    Vector<float> default_decode() const override;
    ColorSpaceFamily const& family() const override { return ColorSpaceFamily::Separation; }

private:
    SeparationColorSpace() = default;
};

}

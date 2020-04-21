
using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Windows.Media;
using MS.Internal.Text.TextInterface;
using System.Threading;

namespace MS.Internal
{
    class Span
    {
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="element">Span element</param>
        /// <param name="length">Span length</param>
        public Span(Object element, int length)
        {
            this.element = element;
            this.length = length;
        }

        /// <summary>
        /// Span element
        /// </summary>
        public Object element;

        /// <summary>
        /// Span length
        /// </summary>
        public int length;
    };

    namespace Text
    {
        namespace TextInterface
        {

            internal class FontFileLoader : IDWriteFontFileLoaderMirror
            {
                private IFontSourceFactory _fontSourceFactory;

                public FontFileLoader(IFontSourceFactory fontSourceFactory)
                {
                    _fontSourceFactory = fontSourceFactory;
                }

                public virtual unsafe int CreateStreamFromKey(
                                              void* fontFileReferenceKey,
                                              uint fontFileReferenceKeySize,
                                              IntPtr* fontFileStream)
                {
                    uint numberOfCharacters = fontFileReferenceKeySize / sizeof(char);
                    if ((fontFileStream == null)
                        || (fontFileReferenceKeySize % sizeof(char) != 0)                      // The fontFileReferenceKeySize must be divisible by sizeof(WCHAR)
                        || (numberOfCharacters <= 1)                                            // The fontFileReferenceKey cannot be less than or equal 1 character as it has to contain the NULL character.
                        || (((char*)fontFileReferenceKey)[numberOfCharacters - 1] != '\0'))    // The fontFileReferenceKey must end with the NULL character
                    {
                        return HRESULT.E_FAIL;
                    }

                    *fontFileStream = IntPtr.Zero;

                    string uriString = new string((char*)fontFileReferenceKey);

                    int hr = 0;

                    try
                    {
                        IFontSource fontSource = _fontSourceFactory.Create(uriString);
                        FontFileStream customFontFileStream = new FontFileStream(fontSource);

                        IntPtr pIDWriteFontFileStreamMirror = Marshal.GetComInterfaceForObject(
                                                                customFontFileStream,
                                                                typeof(IDWriteFontFileStreamMirror));

                        *fontFileStream = pIDWriteFontFileStreamMirror;
                    }
                    catch (System.Exception exception)
                    {
                        hr = System.Runtime.InteropServices.Marshal.GetHRForException(exception);
                    }

                    return hr;
                }
            }

            internal class FontFileStream : IDWriteFontFileStreamMirror
            {
                Stream _fontSourceStream;
                long _lastWriteTime;
                Object _fontSourceStreamLock;

                internal FontFileStream(IFontSource fontSource)
                {
                    _fontSourceStream = fontSource.GetUnmanagedStream();
                    try
                    {
                        _lastWriteTime = fontSource.GetLastWriteTimeUtc().ToFileTimeUtc();
                    }
                    catch (System.ArgumentOutOfRangeException) //The resulting file time would represent a date and time before 12:00 midnight January 1, 1601 C.E. UTC. 
                    {
                        _lastWriteTime = -1;
                    }
                    // Create lock to control access to font source stream.
                    _fontSourceStreamLock = new object();

                }

                public unsafe int ReadFileFragment(void** fragmentStart, [In, MarshalAs(UnmanagedType.U8)] ulong fileOffset, [In, MarshalAs(UnmanagedType.U8)] ulong fragmentSize, [Out] void** fragmentContext)
                {
                    int hr = HRESULT.S_OK;
                    try
                    {
                        if (
                            fragmentContext == null || fragmentStart == null
                            ||
                            fileOffset > long.MaxValue                    // Cannot safely cast to long
                            ||
                            fragmentSize > int.MaxValue                     // fragment size cannot be casted to int
                            ||
                            fileOffset > ulong.MaxValue - fragmentSize           // make sure next sum doesn't overflow
                            ||
                            fileOffset + fragmentSize > (ulong)_fontSourceStream.Length // reading past the end of the Stream
                          )
                        {
                            return HRESULT.E_FAIL;
                        }

                        int fragmentSizeInt = (int)fragmentSize;
                        byte[] buffer = new byte[fragmentSizeInt];

                        // DWrite may call this method from multiple threads. We need to ensure thread safety by making Seek and Read atomic.
                        System.Threading.Monitor.Enter(_fontSourceStreamLock);
                        try
                        {
                            _fontSourceStream.Seek((long)fileOffset, //long
                                                    System.IO.SeekOrigin.Begin);

                            _fontSourceStream.Read(buffer,         //byte[]
                                                    0,              //int
                                                    fragmentSizeInt //int
                                                    );
                        }
                        finally
                        {
                            System.Threading.Monitor.Exit(_fontSourceStreamLock);
                        }

                        GCHandle gcHandle = GCHandle.Alloc(buffer, GCHandleType.Pinned);

                        *fragmentStart = (byte*)(gcHandle.AddrOfPinnedObject().ToPointer());
                        *fragmentContext = GCHandle.ToIntPtr(gcHandle).ToPointer();
                    }
                    catch (System.Exception exception)
                    {
                        hr = System.Runtime.InteropServices.Marshal.GetHRForException(exception);
                    }

                    return hr;
                }

                public unsafe void ReleaseFileFragment([In] void* fragmentContext)
                {
                    if (fragmentContext != null)
                    {
                        GCHandle gcHandle = GCHandle.FromIntPtr((IntPtr)fragmentContext);
                        gcHandle.Free();
                    }
                }

                public unsafe int GetFileSize([Out] ulong* fileSize)
                {
                    if (fileSize == null)
                    {
                        return HRESULT.E_FAIL;
                    }

                    int hr = HRESULT.S_OK;
                    try
                    {
                        *fileSize = (ulong)_fontSourceStream.Length;
                    }
                    catch (System.Exception exception)
                    {
                        hr = System.Runtime.InteropServices.Marshal.GetHRForException(exception);
                    }

                    return hr;
                }

                public unsafe int GetLastWriteTime([Out] ulong* lastWriteTime)
                {
                    if (_lastWriteTime < 0) //The resulting file time would represent a date and time before 12:00 midnight January 1, 1601 C.E. UTC.
                    {
                        return HRESULT.E_FAIL;
                    }
                    if (lastWriteTime == null)
                    {
                        return HRESULT.E_FAIL;
                    }
                    *lastWriteTime = (ulong)_lastWriteTime;
                    return HRESULT.S_OK;
                }
            }

            interface IFontSourceCollection : IEnumerable<IFontSource>
            {
            }

            interface IFontSourceCollectionFactory
            {
                IFontSourceCollection Create(string uri);
            }

            interface IFontSource
            {
                void TestFileOpenable();
                System.IO.UnmanagedMemoryStream GetUnmanagedStream();
                System.DateTime GetLastWriteTimeUtc();
                System.Uri Uri
                {
                    get;
                }

                bool IsComposite
                {
                    get;
                }
            }

            interface IFontSourceFactory
            {
                IFontSource Create(string uri);
            }

            public class LocalizedErrorMsgs
            {
                public static string EnumeratorNotStarted { get; internal set; }
                public static string EnumeratorReachedEnd { get; internal set; }
            }

            public unsafe delegate void* CreateTextAnalysisSink();
            public unsafe delegate void* GetScriptAnalysisList(void* list);
            public unsafe delegate void* GetNumberSubstitutionList(void* list);
            public unsafe delegate int CreateTextAnalysisSource(
                char* text,
                uint length,
                char* culture,
                void* factory,
                bool isRightToLeft,
                char* numberCulture,
                bool ignoreUserOverride,
                uint numberSubstitutionMethod,
                void** ppTextAnalysisSource);

            enum FactoryType
            {
                Shared,
                Isolated
            };

            /// <summary>
            /// The font style enumeration describes the slope style of a font face, such as Normal, Italic or Oblique.
            /// Values other than the ones defined in the enumeration are considered to be invalid, and they are rejected by font API functions.
            /// </summary>
            public enum FontStyle
            {
                /// <summary>
                /// Font slope style : Normal.
                /// </summary>
                Normal = 0,

                /// <summary>
                /// Font slope style : Oblique.
                /// </summary>
                Oblique = 1,

                /// <summary>
                /// Font slope style : Italic.
                /// </summary>
                Italic = 2
            };

            public enum FontStretch
            {
                /// <summary>
                /// Predefined font stretch : Not known (0).
                /// </summary>
                Undefined = 0,

                /// <summary>
                /// Predefined font stretch : Ultra-condensed (1).
                /// </summary>
                UltraCondensed = 1,

                /// <summary>
                /// Predefined font stretch : Extra-condensed (2).
                /// </summary>
                ExtraCondensed = 2,

                /// <summary>
                /// Predefined font stretch : Condensed (3).
                /// </summary>
                Condensed = 3,

                /// <summary>
                /// Predefined font stretch : Semi-condensed (4).
                /// </summary>
                SemiCondensed = 4,

                /// <summary>
                /// Predefined font stretch : Normal (5).
                /// </summary>
                Normal = 5,

                /// <summary>
                /// Predefined font stretch : Medium (5).
                /// </summary>
                Medium = 5,

                /// <summary>
                /// Predefined font stretch : Semi-expanded (6).
                /// </summary>
                SemiExpanded = 6,

                /// <summary>
                /// Predefined font stretch : Expanded (7).
                /// </summary>
                Expanded = 7,

                /// <summary>
                /// Predefined font stretch : Extra-expanded (8).
                /// </summary>
                ExtraExpanded = 8,

                /// <summary>
                /// Predefined font stretch : Ultra-expanded (9).
                /// </summary>
                UltraExpanded = 9
            };

            public enum FontWeight
            {
                /// <summary>
                /// Predefined font weight : Thin (100).
                /// </summary>
                Thin = 100,

                /// <summary>
                /// Predefined font weight : Extra-light (200).
                /// </summary>
                ExtraLight = 200,

                /// <summary>
                /// Predefined font weight : Ultra-light (200).
                /// </summary>
                UltraLight = 200,

                /// <summary>
                /// Predefined font weight : Light (300).
                /// </summary>
                Light = 300,

                /// <summary>
                /// Predefined font weight : Normal (400).
                /// </summary>
                Normal = 400,

                /// <summary>
                /// Predefined font weight : Regular (400).
                /// </summary>
                Regular = 400,

                /// <summary>
                /// Predefined font weight : Medium (500).
                /// </summary>
                Medium = 500,

                /// <summary>
                /// Predefined font weight : Demi-bold (600).
                /// </summary>
                DemiBold = 600,

                /// <summary>
                /// Predefined font weight : Semi-bold (600).
                /// </summary>
                SemiBOLD = 600,

                /// <summary>
                /// Predefined font weight : Bold (700).
                /// </summary>
                Bold = 700,

                /// <summary>
                /// Predefined font weight : Extra-bold (800).
                /// </summary>
                ExtraBold = 800,

                /// <summary>
                /// Predefined font weight : Ultra-bold (800).
                /// </summary>
                UltraBold = 800,

                /// <summary>
                /// Predefined font weight : Black (900).
                /// </summary>
                Black = 900,

                /// <summary>
                /// Predefined font weight : Heavy (900).
                /// </summary>
                Heavy = 900,

                /// <summary>
                /// Predefined font weight : Extra-black (950).
                /// </summary>
                ExtraBlack = 950,

                /// <summary>
                /// Predefined font weight : Ultra-black (950).
                /// </summary>
                UltraBlack = 950
            };

            [StructLayout(LayoutKind.Explicit)]
            public class FontMetrics
            {
                /// <summary>
                /// The number of font design units per em unit.
                /// Font files use their own coordinate system of font design units.
                /// A font design unit is the smallest measurable unit in the em square,
                /// an imaginary square that is used to size and align glyphs.
                /// The concept of em square is used as a reference scale factor when defining font size and device transformation semantics.
                /// The size of one em square is also commonly used to compute the paragraph identation value.
                /// </summary>
                [FieldOffset(0)]
                public ushort DesignUnitsPerEm;

                /// <summary>
                /// Ascent value of the font face in font design units.
                /// Ascent is the distance from the top of font character alignment box to English baseline.
                /// </summary>
                [FieldOffset(2)]
                public ushort Ascent;

                /// <summary>
                /// Descent value of the font face in font design units.
                /// Descent is the distance from the bottom of font character alignment box to English baseline.
                /// </summary>
                [FieldOffset(4)]
                public ushort Descent;

                /// <summary>
                /// Line gap in font design units.
                /// Recommended additional white space to add between lines to improve legibility. The recommended line spacing
                /// (baseline-to-baseline distance) is thus the sum of ascent, descent, and lineGap. The line gap is usually
                /// positive or zero but can be negative, in which case the recommended line spacing is less than the height
                /// of the character alignment box.
                /// </summary>
                [FieldOffset(8)]
                public short LineGap;

                /// <summary>
                /// Cap height value of the font face in font design units.
                /// Cap height is the distance from English baseline to the top of a typical English capital.
                /// Capital "H" is often used as a reference character for the purpose of calculating the cap height value.
                /// </summary>
                [FieldOffset(10)]
                public ushort CapHeight;

                /// <summary>
                /// x-height value of the font face in font design units.
                /// x-height is the distance from English baseline to the top of lowercase letter "x", or a similar lowercase character.
                /// </summary>
                [FieldOffset(12)]
                public ushort XHeight;

                /// <summary>
                /// The underline position value of the font face in font design units.
                /// Underline position is the position of underline relative to the English baseline.
                /// The value is usually made negative in order to place the underline below the baseline.
                /// </summary>
                [FieldOffset(14)]
                public short UnderlinePosition;

                /// <summary>
                /// The suggested underline thickness value of the font face in font design units.
                /// </summary>
                [FieldOffset(16)]
                public ushort UnderlineThickness;

                /// <summary>
                /// The strikethrough position value of the font face in font design units.
                /// Strikethrough position is the position of strikethrough relative to the English baseline.
                /// The value is usually made positive in order to place the strikethrough above the baseline.
                /// </summary>
                [FieldOffset(18)]
                public short StrikethroughPosition;

                /// <summary>
                /// The suggested strikethrough thickness value of the font face in font design units.
                /// </summary>
                [FieldOffset(20)]
                public ushort StrikethroughThickness;

                /// <summary>
                /// The baseline of the font face.
                /// </summary>
                public double Baseline
                {
                    get
                    {
                        return (double)(this.Ascent + this.LineGap * 0.5) / DesignUnitsPerEm;
                    }
                }

                /// <summary>
                /// The linespacing of the font face.
                /// </summary>
                public double LineSpacing
                {
                    get
                    {
                        return (double)(this.Ascent + this.Descent + this.LineGap) / DesignUnitsPerEm;
                    }
                }
            };

            public enum FontFaceType
            {
                /// <summary>
                /// OpenType font face with CFF outlines.
                /// </summary>
                CFF,

                /// <summary>
                /// OpenType font face with TrueType outlines.
                /// </summary>
                TrueType,

                /// <summary>
                /// OpenType font face that is a part of a TrueType collection.
                /// </summary>
                TrueTypeCollection,

                /// <summary>
                /// A Type 1 font face.
                /// </summary>
                Type1,

                /// <summary>
                /// A vector .FON format font face.
                /// </summary>
                Vector,

                /// <summary>
                /// A bitmap .FON format font face.
                /// </summary>
                Bitmap,

                /// <summary>
                /// Font face type is not recognized by the DirectWrite font system.
                /// </summary>
                Unknown
            };

            [System.Flags]
            public enum FontSimulations
            {
                /// <summary>
                /// No simulations are performed.
                /// </summary>
                None = 0x0000,

                /// <summary>
                /// Algorithmic emboldening is performed.
                /// </summary>
                Bold = 0x0001,

                /// <summary>
                /// Algorithmic italicization is performed.
                /// </summary>
                Oblique = 0x0002
            };

            [StructLayout(LayoutKind.Explicit)]
            public struct GlyphOffset
            {
                [FieldOffset(0)]
                public int du;

                [FieldOffset(4)]
                public int dv;
            };

            /// <summary>
            /// Optional adjustment to a glyph's position. A glyph offset changes the position of a glyph without affecting
            /// the pen position. Offsets are in logical, pre-transform units.
            /// </summary>
            [StructLayout(LayoutKind.Sequential)]
            struct DWRITE_GLYPH_OFFSET
            {
                /// <summary>
                /// Offset in the advance direction of the run. A positive advance offset moves the glyph to the right
                /// (in pre-transform coordinates) if the run is left-to-right or to the left if the run is right-to-left.
                /// </summary>
                public float advanceOffset;

                /// <summary>
                /// Offset in the ascent direction, i.e., the direction ascenders point. A positive ascender offset moves
                /// the glyph up (in pre-transform coordinates).
                /// </summary>
                public float ascenderOffset;
            };

            /// <summary>
            /// The DWRITE_MATRIX structure specifies the graphics transform to be applied
            /// to rendered glyphs.
            /// </summary>
            [StructLayout(LayoutKind.Explicit)]
            public struct DWRITE_MATRIX
            {
                /// <summary>
                /// Horizontal scaling / cosine of rotation
                /// </summary>
                [FieldOffset(0)]
                public float m11;

                /// <summary>
                /// Vertical shear / sine of rotation
                /// </summary>
                [FieldOffset(4)]
                public float m12;

                /// <summary>
                /// Horizontal shear / negative sine of rotation
                /// </summary>
                [FieldOffset(8)]
                public float m21;

                /// <summary>
                /// Vertical scaling / cosine of rotation
                /// </summary>
                [FieldOffset(12)]
                public float m22;

                /// <summary>
                /// Horizontal shift (always orthogonal regardless of rotation)
                /// </summary>
                [FieldOffset(16)]
                public float dx;

                /// <summary>
                /// Vertical shift (always orthogonal regardless of rotation)
                /// </summary>
                [FieldOffset(20)]
                public float dy;
            };

            [StructLayout(LayoutKind.Explicit)]
            public struct DWRITE_FONT_METRICS
            {
                /// <summary>
                /// The number of font design units per em unit.
                /// Font files use their own coordinate system of font design units.
                /// A font design unit is the smallest measurable unit in the em square,
                /// an imaginary square that is used to size and align glyphs.
                /// The concept of em square is used as a reference scale factor when defining font size and device transformation semantics.
                /// The size of one em square is also commonly used to compute the paragraph indentation value.
                /// </summary>
                [FieldOffset(0)]
                public ushort designUnitsPerEm;

                /// <summary>
                /// Ascent value of the font face in font design units.
                /// Ascent is the distance from the top of font character alignment box to English baseline.
                /// </summary>
                [FieldOffset(2)]
                public ushort ascent;

                /// <summary>
                /// Descent value of the font face in font design units.
                /// Descent is the distance from the bottom of font character alignment box to English baseline.
                /// </summary>
                [FieldOffset(4)]
                public ushort descent;

                /// <summary>
                /// Line gap in font design units.
                /// Recommended additional white space to add between lines to improve legibility. The recommended line spacing 
                /// (baseline-to-baseline distance) is thus the sum of ascent, descent, and lineGap. The line gap is usually 
                /// positive or zero but can be negative, in which case the recommended line spacing is less than the height
                /// of the character alignment box.
                /// </summary>
                [FieldOffset(6)]
                public short lineGap;

                /// <summary>
                /// Cap height value of the font face in font design units.
                /// Cap height is the distance from English baseline to the top of a typical English capital.
                /// Capital "H" is often used as a reference character for the purpose of calculating the cap height value.
                /// </summary>
                [FieldOffset(8)]
                public ushort capHeight;

                /// <summary>
                /// x-height value of the font face in font design units.
                /// x-height is the distance from English baseline to the top of lowercase letter "x", or a similar lowercase character.
                /// </summary>
                [FieldOffset(10)]
                public ushort xHeight;

                /// <summary>
                /// The underline position value of the font face in font design units.
                /// Underline position is the position of underline relative to the English baseline.
                /// The value is usually made negative in order to place the underline below the baseline.
                /// </summary>
                [FieldOffset(12)]
                public short underlinePosition;

                /// <summary>
                /// The suggested underline thickness value of the font face in font design units.
                /// </summary>
                [FieldOffset(14)]
                public ushort underlineThickness;

                /// <summary>
                /// The strikethrough position value of the font face in font design units.
                /// Strikethrough position is the position of strikethrough relative to the English baseline.
                /// The value is usually made positive in order to place the strikethrough above the baseline.
                /// </summary>
                [FieldOffset(16)]
                public short strikethroughPosition;

                /// <summary>
                /// The suggested strikethrough thickness value of the font face in font design units.
                /// </summary>
                [FieldOffset(18)]
                public ushort strikethroughThickness;
            }

            [StructLayout(LayoutKind.Explicit)]
            public struct GlyphMetrics
            {
                /// <summary>
                /// Specifies the X offset from the glyph origin to the left edge of the black box.
                /// The glyph origin is the current horizontal writing position.
                /// A negative value means the black box extends to the left of the origin (often true for lowercase italic 'f').
                /// </summary>
                [FieldOffset(0)]
                public int LeftSideBearing;

                /// <summary>
                /// Specifies the X offset from the origin of the current glyph to the origin of the next glyph when writing horizontally.
                /// </summary>
                [FieldOffset(4)]
                public int AdvanceWidth;

                /// <summary>
                /// Specifies the X offset from the right edge of the black box to the origin of the next glyph when writing horizontally.
                /// The value is negative when the right edge of the black box overhangs the layout box.
                /// </summary>
                [FieldOffset(8)]
                public int RightSideBearing;

                /// <summary>
                /// Specifies the vertical offset from the vertical origin to the top of the black box.
                /// Thus, a positive value adds whitespace whereas a negative value means the glyph overhangs the top of the layout box.
                /// </summary>
                [FieldOffset(12)]
                public int TopSideBearing;

                /// <summary>
                /// Specifies the Y offset from the vertical origin of the current glyph to the vertical origin of the next glyph when writing vertically.
                /// (Note that the term "origin" by itself denotes the horizontal origin. The vertical origin is different.
                /// Its Y coordinate is specified by verticalOriginY value,
                /// and its X coordinate is half the advanceWidth to the right of the horizontal origin).
                /// </summary>
                [FieldOffset(16)]
                public int AdvanceHeight;

                /// <summary>
                /// Specifies the vertical distance from the black box's bottom edge to the advance height.
                /// Positive when the bottom edge of the black box is within the layout box.
                /// Negative when the bottom edge of black box overhangs the layout box.
                /// </summary>
                [FieldOffset(20)]
                public int BottomSideBearing;

                /// <summary>
                /// Specifies the Y coordinate of a glyph's vertical origin, in the font's design coordinate system.
                /// The y coordinate of a glyph's vertical origin is the sum of the glyph's top side bearing
                /// and the top (i.e. yMax) of the glyph's bounding box.
                /// </summary>
                [FieldOffset(24)]
                public int VerticalOriginY;
            };

            enum InformationalStringID
            {
                /// <summary>
                /// Unspecified name ID.
                /// </summary>
                None,

                /// <summary>
                /// Copyright notice provided by the font.
                /// </summary>
                CopyrightNotice,

                /// <summary>
                /// String containing a version number.
                /// </summary>
                VersionStrings,

                /// <summary>
                /// Trademark information provided by the font.
                /// </summary>
                Trademark,

                /// <summary>
                /// Name of the font manufacturer.
                /// </summary>
                Manufacturer,

                /// <summary>
                /// Name of the font designer.
                /// </summary>
                Designer,

                /// <summary>
                /// URL of font designer (with protocol, e.g., http://, ftp://).
                /// </summary>
                DesignerURL,

                /// <summary>
                /// Description of the font. Can contain revision information, usage recommendations, history, features, etc.
                /// </summary>
                Description,

                /// <summary>
                /// URL of font vendor (with protocol, e.g., http://, ftp://). If a unique serial number is embedded in the URL, it can be used to register the font.
                /// </summary>
                FontVendorURL,

                /// <summary>
                /// Description of how the font may be legally used, or different example scenarios for licensed use. This field should be written in plain language, not legalese.
                /// </summary>
                LicenseDescription,

                /// <summary>
                /// URL where additional licensing information can be found.
                /// </summary>
                LicenseInfoURL,

                /// <summary>
                /// GDI-compatible family name. Because GDI allows a maximum of four fonts per family, fonts in the same family may have different GDI-compatible family names
                /// (e.g., "Arial", "Arial Narrow", "Arial Black").
                /// </summary>
                WIN32FamilyNames,

                /// <summary>
                /// GDI-compatible subfamily name.
                /// </summary>
                Win32SubFamilyNames,

                /// <summary>
                /// Family name preferred by the designer. This enables font designers to group more than four fonts in a single family without losing compatibility with
                /// GDI. This name is typically only present if it differs from the GDI-compatible family name.
                /// </summary>
                PreferredFamilyNames,

                /// <summary>
                /// Subfamily name preferred by the designer. This name is typically only present if it differs from the GDI-compatible subfamily name.
                /// </summary>
                PreferredSubFamilyNames,

                /// <summary>
                /// Sample text. This can be the font name or any other text that the designer thinks is the best example to display the font in.
                /// </summary>
                SampleText
            }

            public enum OpenTypeTableTag
            {
                TTO_GSUB = ('G' | ('S' << 8) | ('U' << 16) | ('B' << 24)),        /* 'GSUB' */
                TTO_GPOS = ('G' | ('P' << 8) | ('O' << 16) | ('S' << 24)),        /* 'GPOS' */
                TTO_GDEF = ('G' | ('D' << 8) | ('E' << 16) | ('F' << 24)),        /* 'GDEF' */
            };

            /// <summary>
            /// Typographic feature of text supplied by the font.
            /// </summary>
            public enum DWriteFontFeatureTag
            {
                AlternativeFractions = 0x63726661, // 'afrc'
                PetiteCapitalsFromCapitals = 0x63703263, // 'c2pc'
                SmallCapitalsFromCapitals = 0x63733263, // 'c2sc'
                ContextualAlternates = 0x746c6163, // 'calt'
                CaseSensitiveForms = 0x65736163, // 'case'
                GlyphCompositionDecomposition = 0x706d6363, // 'ccmp'
                ContextualLigatures = 0x67696c63, // 'clig'
                CapitalSpacing = 0x70737063, // 'cpsp'
                ContextualSwash = 0x68777363, // 'cswh'
                CursivePositioning = 0x73727563, // 'curs'
                Default = 0x746c6664, // 'dflt'
                DiscretionaryLigatures = 0x67696c64, // 'dlig'
                ExpertForms = 0x74707865, // 'expt'
                Fractions = 0x63617266, // 'frac'
                FullWidth = 0x64697766, // 'fwid'
                HalfForms = 0x666c6168, // 'half'
                HalantForms = 0x6e6c6168, // 'haln'
                AlternateHalfWidth = 0x746c6168, // 'halt'
                HistoricalForms = 0x74736968, // 'hist'
                HorizontalKanaAlternates = 0x616e6b68, // 'hkna'
                HistoricalLigatures = 0x67696c68, // 'hlig'
                HalfWidth = 0x64697768, // 'hwid'
                HojoKanjiForms = 0x6f6a6f68, // 'hojo'
                JIS04Forms = 0x3430706a, // 'jp04'
                JIS78Forms = 0x3837706a, // 'jp78'
                JIS83Forms = 0x3338706a, // 'jp83'
                JIS90Forms = 0x3039706a, // 'jp90'
                Kerning = 0x6e72656b, // 'kern'
                StandardLigatures = 0x6167696c, // 'liga'
                LiningFigures = 0x6d756e6c, // 'lnum'
                LocalizedForms = 0x6c636f6c, // 'locl'
                MarkPositioning = 0x6b72616d, // 'mark'
                MathematicalGreek = 0x6b72676d, // 'mgrk'
                MarkToMarkPositioning = 0x6b6d6b6d, // 'mkmk'
                AlternateAnnotationForms = 0x746c616e, // 'nalt'
                NLCKanjiForms = 0x6b636c6e, // 'nlck'
                OldStyleFigures = 0x6d756e6f, // 'onum'
                Ordinals = 0x6e64726f, // 'ordn'
                ProportionalAlternateWidth = 0x746c6170, // 'palt'
                PetiteCapitals = 0x70616370, // 'pcap'
                ProportionalFigures = 0x6d756e70, // 'pnum'
                ProportionalWidths = 0x64697770, // 'pwid'
                QuarterWidths = 0x64697771, // 'qwid'
                RequiredLigatures = 0x67696c72, // 'rlig'
                RubyNotationForms = 0x79627572, // 'ruby'
                StylisticAlternates = 0x746c6173, // 'salt'
                ScientificInferiors = 0x666e6973, // 'sinf'
                SmallCapitals = 0x70636d73, // 'smcp'
                SimplifiedForms = 0x6c706d73, // 'smpl'
                StylisticSet1 = 0x31307373, // 'ss01'
                StylisticSet2 = 0x32307373, // 'ss02'
                StylisticSet3 = 0x33307373, // 'ss03'
                StylisticSet4 = 0x34307373, // 'ss04'
                StylisticSet5 = 0x35307373, // 'ss05'
                StylisticSet6 = 0x36307373, // 'ss06'
                StylisticSet7 = 0x37307373, // 'ss07'
                StylisticSet8 = 0x38307373, // 'ss08'
                StylisticSet9 = 0x39307373, // 'ss09'
                StylisticSet10 = 0x30317373, // 'ss10'
                StylisticSet11 = 0x31317373, // 'ss11'
                StylisticSet12 = 0x32317373, // 'ss12'
                StylisticSet13 = 0x33317373, // 'ss13'
                StylisticSet14 = 0x34317373, // 'ss14'
                StylisticSet15 = 0x35317373, // 'ss15'
                StylisticSet16 = 0x36317373, // 'ss16'
                StylisticSet17 = 0x37317373, // 'ss17'
                StylisticSet18 = 0x38317373, // 'ss18'
                StylisticSet19 = 0x39317373, // 'ss19'
                StylisticSet20 = 0x30327373, // 'ss20'
                Subscript = 0x73627573, // 'subs'
                Superscript = 0x73707573, // 'sups'
                Swash = 0x68737773, // 'swsh'
                Titling = 0x6c746974, // 'titl'
                TraditionalNameForms = 0x6d616e74, // 'tnam'
                TabularFigures = 0x6d756e74, // 'tnum'
                TraditionalForms = 0x64617274, // 'trad'
                ThirdWidths = 0x64697774, // 'twid'
                Unicase = 0x63696e75, // 'unic'
                SlashedZero = 0x6f72657a, // 'zero'
            };

            [StructLayout(LayoutKind.Sequential)]
            public struct DWriteFontFeature
            {
                /// <summary>
                /// The feature OpenType name identifier.
                /// </summary>
                public DWriteFontFeatureTag nameTag;

                /// <summary>
                /// Execution parameter of the feature.
                /// </summary>
                /// <remarks>
                /// The parameter should be non-zero to enable the feature.  Once enabled, a feature can't be disabled again within
                /// the same range.  Features requiring a selector use this value to indicate the selector index.
                /// </remarks>
                public uint parameter;

                public DWriteFontFeature(DWriteFontFeatureTag dwriteNameTag, uint dwriteParameter)
                {
                    nameTag = dwriteNameTag;
                    parameter = dwriteParameter;
                }
            };

            public class DWriteTypeConverter
            {
                internal static DWRITE_FACTORY_TYPE Convert(FactoryType factoryType)
                {
                    switch (factoryType)
                    {
                        case FactoryType.Shared:
                            return DWRITE_FACTORY_TYPE.DWRITE_FACTORY_TYPE_SHARED;
                        case FactoryType.Isolated:
                            return DWRITE_FACTORY_TYPE.DWRITE_FACTORY_TYPE_ISOLATED;
                        default:
                            throw new InvalidOperationException();
                    }
                }

                internal static DWRITE_FONT_SIMULATIONS Convert(FontSimulations fontSimulations)
                {
                    switch (fontSimulations)
                    {
                        case FontSimulations.Bold:
                            return DWRITE_FONT_SIMULATIONS.DWRITE_FONT_SIMULATIONS_BOLD;
                        case FontSimulations.Oblique:
                            return DWRITE_FONT_SIMULATIONS.DWRITE_FONT_SIMULATIONS_OBLIQUE;
                        case (FontSimulations.Bold | FontSimulations.Oblique):
                            return (DWRITE_FONT_SIMULATIONS.DWRITE_FONT_SIMULATIONS_BOLD | DWRITE_FONT_SIMULATIONS.DWRITE_FONT_SIMULATIONS_OBLIQUE);
                        case FontSimulations.None:
                            return DWRITE_FONT_SIMULATIONS.DWRITE_FONT_SIMULATIONS_NONE;
                        default:
                            throw new System.InvalidOperationException();
                    }
                }

                internal static FontMetrics Convert(DWRITE_FONT_METRICS dwriteFontMetrics)
                {
                    FontMetrics fontMetrics = new FontMetrics();

                    fontMetrics.Ascent = dwriteFontMetrics.ascent;
                    fontMetrics.CapHeight = dwriteFontMetrics.capHeight;
                    fontMetrics.Descent = dwriteFontMetrics.descent;
                    fontMetrics.DesignUnitsPerEm = dwriteFontMetrics.designUnitsPerEm;
                    fontMetrics.LineGap = dwriteFontMetrics.lineGap;
                    fontMetrics.StrikethroughPosition = dwriteFontMetrics.strikethroughPosition;
                    fontMetrics.StrikethroughThickness = dwriteFontMetrics.strikethroughThickness;
                    fontMetrics.UnderlinePosition = dwriteFontMetrics.underlinePosition;
                    fontMetrics.UnderlineThickness = dwriteFontMetrics.underlineThickness;
                    fontMetrics.XHeight = dwriteFontMetrics.xHeight;

                    return fontMetrics;
                }

                internal static FontFaceType Convert(DWRITE_FONT_FACE_TYPE fontFaceType)
                {
                    switch (fontFaceType)
                    {
                        case DWRITE_FONT_FACE_TYPE.DWRITE_FONT_FACE_TYPE_BITMAP:
                            return FontFaceType.Bitmap;
                        case DWRITE_FONT_FACE_TYPE.DWRITE_FONT_FACE_TYPE_CFF:
                            return FontFaceType.CFF;
                        case DWRITE_FONT_FACE_TYPE.DWRITE_FONT_FACE_TYPE_TRUETYPE:
                            return FontFaceType.TrueType;
                        case DWRITE_FONT_FACE_TYPE.DWRITE_FONT_FACE_TYPE_TRUETYPE_COLLECTION:
                            return FontFaceType.TrueTypeCollection;
                        case DWRITE_FONT_FACE_TYPE.DWRITE_FONT_FACE_TYPE_TYPE1:
                            return FontFaceType.Type1;
                        case DWRITE_FONT_FACE_TYPE.DWRITE_FONT_FACE_TYPE_VECTOR:
                            return FontFaceType.Vector;
                        case DWRITE_FONT_FACE_TYPE.DWRITE_FONT_FACE_TYPE_UNKNOWN:
                            return FontFaceType.Unknown;

                        // The following were added to DWrite.h in the Win8 SDK, but are not currently supported by WPF.
                        case DWRITE_FONT_FACE_TYPE.DWRITE_FONT_FACE_TYPE_RAW_CFF: // return FontFaceType::RawCFF;

                        default:
                            throw new System.InvalidOperationException();
                    }
                }

                internal static FontSimulations Convert(DWRITE_FONT_SIMULATIONS fontSimulations)
                {
                    switch (fontSimulations)
                    {
                        case DWRITE_FONT_SIMULATIONS.DWRITE_FONT_SIMULATIONS_BOLD:
                            return FontSimulations.Bold;
                        case DWRITE_FONT_SIMULATIONS.DWRITE_FONT_SIMULATIONS_OBLIQUE:
                            return FontSimulations.Oblique;
                        case DWRITE_FONT_SIMULATIONS.DWRITE_FONT_SIMULATIONS_NONE:
                            return FontSimulations.None;
                        // We did have (DWRITE_FONT_SIMULATIONS_BOLD | DWRITE_FONT_SIMULATIONS_OBLIQUE) as a switch case, but the compiler
                        // started throwing C2051 on this when I ported from WPFText to WPFDWrite. Probably some compiler or build setting 
                        // change caused this. Just moved it to the default case instead.
                        default:
                            if (fontSimulations == (DWRITE_FONT_SIMULATIONS.DWRITE_FONT_SIMULATIONS_BOLD | DWRITE_FONT_SIMULATIONS.DWRITE_FONT_SIMULATIONS_OBLIQUE))
                                return (FontSimulations.Bold | FontSimulations.Oblique);
                            else
                                throw new InvalidOperationException();
                    }
                }

                internal static FontStretch Convert(DWRITE_FONT_STRETCH fontStretch)
                {
                    // The commented cases are here only for completeness so that the code captures all the possible enum values.
                    // However, these enum values are commented out since there are several enums having the same value.
                    // For example, both Normal and Medium have a value of 5.
                    switch (fontStretch)
                    {
                        case DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_UNDEFINED:
                            return FontStretch.Undefined;
                        case DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_ULTRA_CONDENSED:
                            return FontStretch.UltraCondensed;
                        case DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_EXTRA_CONDENSED:
                            return FontStretch.ExtraCondensed;
                        case DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_CONDENSED:
                            return FontStretch.Condensed;
                        case DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_SEMI_CONDENSED:
                            return FontStretch.SemiCondensed;
                        case DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_NORMAL:
                            return FontStretch.Normal;
                        //case DWRITE_FONT_STRETCH_MEDIUM          : return FontStretch::Medium;
                        case DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_SEMI_EXPANDED:
                            return FontStretch.SemiExpanded;
                        case DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_EXPANDED:
                            return FontStretch.Expanded;
                        case DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_EXTRA_EXPANDED:
                            return FontStretch.ExtraExpanded;
                        case DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_ULTRA_EXPANDED:
                            return FontStretch.UltraExpanded;
                        default:
                            throw new InvalidOperationException();
                    }
                }

                internal static DWRITE_FONT_STRETCH Convert(FontStretch fontStretch)
                {
                    // The commented cases are here only for completeness so that the code captures all the possible enum values.
                    // However, these enum values are commented out since there are several enums having the same value.
                    // For example, both Normal and Medium have a value of 5.
                    switch (fontStretch)
                    {
                        case FontStretch.Undefined:
                            return DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_UNDEFINED;
                        case FontStretch.UltraCondensed:
                            return DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_ULTRA_CONDENSED;
                        case FontStretch.ExtraCondensed:
                            return DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_EXTRA_CONDENSED;
                        case FontStretch.Condensed:
                            return DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_CONDENSED;
                        case FontStretch.SemiCondensed:
                            return DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_SEMI_CONDENSED;
                        case FontStretch.Normal:
                            return DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_NORMAL;
                        //case FontStretch::Medium         : return DWRITE_FONT_STRETCH_MEDIUM;
                        case FontStretch.SemiExpanded:
                            return DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_SEMI_EXPANDED;
                        case FontStretch.Expanded:
                            return DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_EXPANDED;
                        case FontStretch.ExtraExpanded:
                            return DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_EXTRA_EXPANDED;
                        case FontStretch.UltraExpanded:
                            return DWRITE_FONT_STRETCH.DWRITE_FONT_STRETCH_ULTRA_EXPANDED;
                        default:
                            throw new InvalidOperationException();
                    }
                }

                internal static FontStyle Convert(DWRITE_FONT_STYLE fontStyle)
                {
                    switch (fontStyle)
                    {
                        case DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_NORMAL:
                            return FontStyle.Normal;
                        case DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_ITALIC:
                            return FontStyle.Italic;
                        case DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_OBLIQUE:
                            return FontStyle.Oblique;
                        default:
                            throw new InvalidOperationException();
                    }
                }

                static internal DWRITE_FONT_STYLE Convert(FontStyle fontStyle)
                {
                    switch (fontStyle)
                    {
                        case FontStyle.Normal:
                            return DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_NORMAL;
                        case FontStyle.Italic:
                            return DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_ITALIC;
                        case FontStyle.Oblique:
                            return DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_OBLIQUE;
                        default:
                            throw new InvalidOperationException();
                    }
                }

                internal static DWRITE_INFORMATIONAL_STRING_ID Convert(InformationalStringID informationStringID)
                {
                    switch (informationStringID)
                    {
                        case InformationalStringID.None:
                            return DWRITE_INFORMATIONAL_STRING_ID.DWRITE_INFORMATIONAL_STRING_NONE;
                        case InformationalStringID.CopyrightNotice:
                            return DWRITE_INFORMATIONAL_STRING_ID.DWRITE_INFORMATIONAL_STRING_COPYRIGHT_NOTICE;
                        case InformationalStringID.VersionStrings:
                            return DWRITE_INFORMATIONAL_STRING_ID.DWRITE_INFORMATIONAL_STRING_VERSION_STRINGS;
                        case InformationalStringID.Trademark:
                            return DWRITE_INFORMATIONAL_STRING_ID.DWRITE_INFORMATIONAL_STRING_TRADEMARK;
                        case InformationalStringID.Manufacturer:
                            return DWRITE_INFORMATIONAL_STRING_ID.DWRITE_INFORMATIONAL_STRING_MANUFACTURER;
                        case InformationalStringID.Designer:
                            return DWRITE_INFORMATIONAL_STRING_ID.DWRITE_INFORMATIONAL_STRING_DESIGNER;
                        case InformationalStringID.DesignerURL:
                            return DWRITE_INFORMATIONAL_STRING_ID.DWRITE_INFORMATIONAL_STRING_DESIGNER_URL;
                        case InformationalStringID.Description:
                            return DWRITE_INFORMATIONAL_STRING_ID.DWRITE_INFORMATIONAL_STRING_DESCRIPTION;
                        case InformationalStringID.FontVendorURL:
                            return DWRITE_INFORMATIONAL_STRING_ID.DWRITE_INFORMATIONAL_STRING_FONT_VENDOR_URL;
                        case InformationalStringID.LicenseDescription:
                            return DWRITE_INFORMATIONAL_STRING_ID.DWRITE_INFORMATIONAL_STRING_LICENSE_DESCRIPTION;
                        case InformationalStringID.LicenseInfoURL:
                            return DWRITE_INFORMATIONAL_STRING_ID.DWRITE_INFORMATIONAL_STRING_LICENSE_INFO_URL;
                        case InformationalStringID.WIN32FamilyNames:
                            return DWRITE_INFORMATIONAL_STRING_ID.DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES;
                        case InformationalStringID.Win32SubFamilyNames:
                            return DWRITE_INFORMATIONAL_STRING_ID.DWRITE_INFORMATIONAL_STRING_WIN32_SUBFAMILY_NAMES;
                        case InformationalStringID.PreferredFamilyNames:
                            return DWRITE_INFORMATIONAL_STRING_ID.DWRITE_INFORMATIONAL_STRING_PREFERRED_FAMILY_NAMES;
                        case InformationalStringID.PreferredSubFamilyNames:
                            return DWRITE_INFORMATIONAL_STRING_ID.DWRITE_INFORMATIONAL_STRING_PREFERRED_SUBFAMILY_NAMES;
                        case InformationalStringID.SampleText:
                            return DWRITE_INFORMATIONAL_STRING_ID.DWRITE_INFORMATIONAL_STRING_SAMPLE_TEXT;

                        // The following were added to DWrite.h in the Win8 SDK, but are not currently supported by WPF.
                        // case InformationalStringID::PostscriptCidName       : return DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_CID_NAME;
                        // case InformationalStringID::PostscriptName          : return DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_NAME;
                        // case InformationalStringID::FullName                : return DWRITE_INFORMATIONAL_STRING_FULL_NAME;

                        default:
                            throw new InvalidOperationException();
                    }
                }


                internal static FontWeight Convert(DWRITE_FONT_WEIGHT fontWeight)
                {
                    // The commented cases are here only for completeness so that the code captures all the possible enum values.
                    // However, these enum values are commented out since there are several enums having the same value.
                    // For example, both Normal and Regular have a value of 400.
                    switch (fontWeight)
                    {
                        case DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_THIN:
                            return FontWeight.Thin;
                        case DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_EXTRA_LIGHT:
                            return FontWeight.ExtraLight;
                        //case DWRITE_FONT_WEIGHT_ULTRA_LIGHT : return FontWeight::UltraLight;
                        case DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_LIGHT:
                            return FontWeight.Light;
                        case DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_NORMAL:
                            return FontWeight.Normal;
                        //case DWRITE_FONT_WEIGHT_REGULAR     : return FontWeight::Regular;
                        case DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_MEDIUM:
                            return FontWeight.Medium;
                        //case DWRITE_FONT_WEIGHT_DEMI_BOLD   : return FontWeight::DemiBold;
                        case DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_SEMI_BOLD:
                            return FontWeight.SemiBOLD;
                        case DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_BOLD:
                            return FontWeight.Bold;
                        case DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_EXTRA_BOLD:
                            return FontWeight.ExtraBold;
                        //case DWRITE_FONT_WEIGHT_ULTRA_BOLD  : return FontWeight::UltraBold;
                        case DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_BLACK:
                            return FontWeight.Black;
                        //case DWRITE_FONT_WEIGHT_HEAVY       : return FontWeight::Heavy;
                        case DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_EXTRA_BLACK:
                            return FontWeight.ExtraBlack;
                        //case DWRITE_FONT_WEIGHT_ULTRA_BLACK : return FontWeight::UltraBlack;

                        case DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_SEMI_LIGHT: // let it fall through - because I don't know what else to do with it
                        default:
                            int weight = (int)fontWeight;
                            if (weight >= 1 && weight <= 999)
                            {
                                return (FontWeight)fontWeight;
                            }
                            else
                            {
                                throw new InvalidOperationException();
                            }
                    }
                }

                internal static DWRITE_FONT_WEIGHT Convert(FontWeight fontWeight)
                {
                    // The commented cases are here only for completeness so that the code captures all the possible enum values.
                    // However, these enum values are commented out since there are several enums having the same value.
                    // For example, both Normal and Regular have a value of 400.
                    switch (fontWeight)
                    {
                        case FontWeight.Thin:
                            return DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_THIN;
                        case FontWeight.ExtraLight:
                            return DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_EXTRA_LIGHT;
                        //case FontWeight::UltraLight : return DWRITE_FONT_WEIGHT_ULTRA_LIGHT;
                        case FontWeight.Light:
                            return DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_LIGHT;
                        case FontWeight.Normal:
                            return DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_NORMAL;
                        //case FontWeight::Regular    : return DWRITE_FONT_WEIGHT_REGULAR;
                        case FontWeight.Medium:
                            return DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_MEDIUM;
                        //case FontWeight::DemiBold   : return DWRITE_FONT_WEIGHT_DEMI_BOLD;
                        case FontWeight.SemiBOLD:
                            return DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_SEMI_BOLD;
                        case FontWeight.Bold:
                            return DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_BOLD;
                        case FontWeight.ExtraBold:
                            return DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_EXTRA_BOLD;
                        //case FontWeight::UltraBold  : return DWRITE_FONT_WEIGHT_ULTRA_BOLD;
                        case FontWeight.Black:
                            return DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_BLACK;
                        //case FontWeight::Heavy      : return DWRITE_FONT_WEIGHT_HEAVY;
                        case FontWeight.ExtraBlack:
                            return DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_EXTRA_BLACK;
                        //case FontWeight::UltraBlack : return DWRITE_FONT_WEIGHT_ULTRA_BLACK;
                        default:
                            int weight = (int)fontWeight;
                            if (weight >= 1 && weight <= 999)
                            {
                                return (DWRITE_FONT_WEIGHT)fontWeight;
                            }
                            else
                            {
                                throw new InvalidOperationException();
                            }
                    }
                }


                internal static ushort Convert(TextFormattingMode measuringMode)
                {
                    switch (measuringMode)
                    {
                        case TextFormattingMode.Ideal:
                            return DWRITE_MEASURING_MODE_NATURAL;
                        case TextFormattingMode.Display:
                            return DWRITE_MEASURING_MODE_GDI_CLASSIC;
                        default:
                            throw new System.InvalidOperationException();
                    }
                }

                internal const int DWRITE_MEASURING_MODE_NATURAL = 0;
                internal const int DWRITE_MEASURING_MODE_GDI_CLASSIC = 1;
            }

            class ItemProps
            {
                public bool HasExtendedCharacter
                {
                    get { return _hasExtendedCharacter; }
                }
                public bool IsLatin
                {
                    get { return _isLatin; }
                }
                public bool IsIndic
                {
                    get { return _isIndic; }
                }
                public bool HasCombiningMark
                {
                    get { return _hasCombiningMark; }
                }
                public bool NeedsCaretInfo
                {
                    get { return _needsCaretInfo; }
                }
                public CultureInfo DigitCulture
                {
                    get { return _digitCulture; }
                }

                public unsafe DWRITE_SCRIPT_ANALYSIS* ScriptAnalysis
                {
                    get { return _scriptAnalysis; }
                }

                public IntPtr NumberSubstitutionNoAddRef { get { return _numberSubstitution; } }

                public unsafe bool CanShapeTogether(ItemProps other)
                {
                    bool canShapeTogether = (_numberSubstitution == other._numberSubstitution // They must have the same number substitution properties.
                        &&
                        ((_scriptAnalysis == null && other._scriptAnalysis == null)
                        ||
                        (_scriptAnalysis != null && other._scriptAnalysis != null &&
                         _scriptAnalysis->script == other._scriptAnalysis->script &&         // They must have the same DWRITE_SCRIPT_ANALYSIS.
                         _scriptAnalysis->shapes == other._scriptAnalysis->shapes)));

                    return canShapeTogether;
                }

                public unsafe static ItemProps Create(
                    DWRITE_SCRIPT_ANALYSIS* scriptAnalysis,
                    IntPtr numberSubstitution,
                    CultureInfo digitCulture,
                    bool hasCombiningMark,
                    bool needsCaretInfo,
                    bool hasExtendedCharacter,
                    bool isIndic,
                    bool isLatin
                    )
                {
                    ItemProps result = new ItemProps();

                    result._digitCulture = digitCulture;
                    result._hasCombiningMark = hasCombiningMark;
                    result._hasExtendedCharacter = hasExtendedCharacter;
                    result._needsCaretInfo = needsCaretInfo;
                    result._isIndic = isIndic;
                    result._isLatin = isLatin;

                    if (scriptAnalysis != null)
                    {
                        result._scriptAnalysis = scriptAnalysis;
                    }

                    result._numberSubstitution = numberSubstitution;
                    if (numberSubstitution != IntPtr.Zero)
                    {
                        //NativeMethods.NumberSubstitutionAddRef(numberSubstitution);
                    }

                    return result;
                }

                private CultureInfo _digitCulture;
                private bool _hasCombiningMark;
                private bool _needsCaretInfo;
                private bool _hasExtendedCharacter;
                private bool _isIndic;
                private bool _isLatin;
                System.IntPtr _numberSubstitution;
                unsafe DWRITE_SCRIPT_ANALYSIS* _scriptAnalysis;
            }

            /// <summary>
            /// Specifies the type of DirectWrite factory object.
            /// DirectWrite factory contains internal state such as font loader registration and cached font data.
            /// In most cases it is recommended to use the shared factory object, because it allows multiple components
            /// that use DirectWrite to share internal DirectWrite state and reduce memory usage.
            /// However, there are cases when it is desirable to reduce the impact of a component,
            /// such as a plug-in from an untrusted source, on the rest of the process by sandboxing and isolating it
            /// from the rest of the process components. In such cases, it is recommended to use an isolated factory for the sandboxed
            /// component.
            /// </summary>
            enum DWRITE_FACTORY_TYPE
            {
                /// <summary>
                /// Shared factory allow for re-use of cached font data across multiple in process components.
                /// Such factories also take advantage of cross process font caching components for better performance.
                /// </summary>
                DWRITE_FACTORY_TYPE_SHARED,

                /// <summary>
                /// Objects created from the isolated factory do not interact with internal DirectWrite state from other components.
                /// </summary>
                DWRITE_FACTORY_TYPE_ISOLATED
            };

            /// <summary>
            /// The type of a font represented by a single font file.
            /// Font formats that consist of multiple files, e.g. Type 1 .PFM and .PFB, have
            /// separate enum values for each of the file type.
            /// </summary>
            enum DWRITE_FONT_FILE_TYPE
            {
                /// <summary>
                /// Font type is not recognized by the DirectWrite font system.
                /// </summary>
                DWRITE_FONT_FILE_TYPE_UNKNOWN,

                /// <summary>
                /// OpenType font with CFF outlines.
                /// </summary>
                DWRITE_FONT_FILE_TYPE_CFF,

                /// <summary>
                /// OpenType font with TrueType outlines.
                /// </summary>
                DWRITE_FONT_FILE_TYPE_TRUETYPE,

                /// <summary>
                /// OpenType font that contains a TrueType collection.
                /// </summary>
                DWRITE_FONT_FILE_TYPE_OPENTYPE_COLLECTION,

                /// <summary>
                /// Type 1 PFM font.
                /// </summary>
                DWRITE_FONT_FILE_TYPE_TYPE1_PFM,

                /// <summary>
                /// Type 1 PFB font.
                /// </summary>
                DWRITE_FONT_FILE_TYPE_TYPE1_PFB,

                /// <summary>
                /// Vector .FON font.
                /// </summary>
                DWRITE_FONT_FILE_TYPE_VECTOR,

                /// <summary>
                /// Bitmap .FON font.
                /// </summary>
                DWRITE_FONT_FILE_TYPE_BITMAP,

                // The following name is obsolete, but kept as an alias to avoid breaking existing code.
                DWRITE_FONT_FILE_TYPE_TRUETYPE_COLLECTION = DWRITE_FONT_FILE_TYPE_OPENTYPE_COLLECTION,
            };

            /// <summary>
            /// The file format of a complete font face.
            /// Font formats that consist of multiple files, e.g. Type 1 .PFM and .PFB, have
            /// a single enum entry.
            /// </summary>
            enum DWRITE_FONT_FACE_TYPE
            {
                /// <summary>
                /// OpenType font face with CFF outlines.
                /// </summary>
                DWRITE_FONT_FACE_TYPE_CFF,

                /// <summary>
                /// OpenType font face with TrueType outlines.
                /// </summary>
                DWRITE_FONT_FACE_TYPE_TRUETYPE,

                /// <summary>
                /// OpenType font face that is a part of a TrueType or CFF collection.
                /// </summary>
                DWRITE_FONT_FACE_TYPE_OPENTYPE_COLLECTION,

                /// <summary>
                /// A Type 1 font face.
                /// </summary>
                DWRITE_FONT_FACE_TYPE_TYPE1,

                /// <summary>
                /// A vector .FON format font face.
                /// </summary>
                DWRITE_FONT_FACE_TYPE_VECTOR,

                /// <summary>
                /// A bitmap .FON format font face.
                /// </summary>
                DWRITE_FONT_FACE_TYPE_BITMAP,

                /// <summary>
                /// Font face type is not recognized by the DirectWrite font system.
                /// </summary>
                DWRITE_FONT_FACE_TYPE_UNKNOWN,

                /// <summary>
                /// The font data includes only the CFF table from an OpenType CFF font.
                /// This font face type can be used only for embedded fonts (i.e., custom
                /// font file loaders) and the resulting font face object supports only the
                /// minimum functionality necessary to render glyphs.
                /// </summary>
                DWRITE_FONT_FACE_TYPE_RAW_CFF,

                // The following name is obsolete, but kept as an alias to avoid breaking existing code.
                DWRITE_FONT_FACE_TYPE_TRUETYPE_COLLECTION = DWRITE_FONT_FACE_TYPE_OPENTYPE_COLLECTION,
            };

            /// <summary>
            /// The informational string enumeration identifies a string in a font.
            /// </summary>
            enum DWRITE_INFORMATIONAL_STRING_ID
            {
                /// <summary>
                /// Unspecified name ID.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_NONE,

                /// <summary>
                /// Copyright notice provided by the font.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_COPYRIGHT_NOTICE,

                /// <summary>
                /// String containing a version number.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_VERSION_STRINGS,

                /// <summary>
                /// Trademark information provided by the font.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_TRADEMARK,

                /// <summary>
                /// Name of the font manufacturer.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_MANUFACTURER,

                /// <summary>
                /// Name of the font designer.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_DESIGNER,

                /// <summary>
                /// URL of font designer (with protocol, e.g., http://, ftp://).
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_DESIGNER_URL,

                /// <summary>
                /// Description of the font. Can contain revision information, usage recommendations, history, features, etc.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_DESCRIPTION,

                /// <summary>
                /// URL of font vendor (with protocol, e.g., http://, ftp://). If a unique serial number is embedded in the URL, it can be used to register the font.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_FONT_VENDOR_URL,

                /// <summary>
                /// Description of how the font may be legally used, or different example scenarios for licensed use. This field should be written in plain language, not legalese.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_LICENSE_DESCRIPTION,

                /// <summary>
                /// URL where additional licensing information can be found.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_LICENSE_INFO_URL,

                /// <summary>
                /// GDI-compatible family name. Because GDI allows a maximum of four fonts per family, fonts in the same family may have different GDI-compatible family names
                /// (e.g., "Arial", "Arial Narrow", "Arial Black").
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES,

                /// <summary>
                /// GDI-compatible subfamily name.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_WIN32_SUBFAMILY_NAMES,

                /// <summary>
                /// Typographic family name preferred by the designer. This enables font designers to group more than four fonts in a single family without losing compatibility with
                /// GDI. This name is typically only present if it differs from the GDI-compatible family name.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_TYPOGRAPHIC_FAMILY_NAMES,

                /// <summary>
                /// Typographic subfamily name preferred by the designer. This name is typically only present if it differs from the GDI-compatible subfamily name. 
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_TYPOGRAPHIC_SUBFAMILY_NAMES,

                /// <summary>
                /// Sample text. This can be the font name or any other text that the designer thinks is the best example to display the font in.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_SAMPLE_TEXT,

                /// <summary>
                /// The full name of the font, e.g. "Arial Bold", from name id 4 in the name table.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_FULL_NAME,

                /// <summary>
                /// The postscript name of the font, e.g. "GillSans-Bold" from name id 6 in the name table.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_NAME,

                /// <summary>
                /// The postscript CID findfont name, from name id 20 in the name table.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_CID_NAME,

                /// <summary>
                /// Family name for the weight-stretch-style model.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_WEIGHT_STRETCH_STYLE_FAMILY_NAME,

                /// <summary>
                /// Script/language tag to identify the scripts or languages that the font was
                /// primarily designed to support. See DWRITE_FONT_PROPERTY_ID_DESIGN_SCRIPT_LANGUAGE_TAG
                /// for a longer description.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_DESIGN_SCRIPT_LANGUAGE_TAG,

                /// <summary>
                /// Script/language tag to identify the scripts or languages that the font declares
                /// it is able to support.
                /// </summary>
                DWRITE_INFORMATIONAL_STRING_SUPPORTED_SCRIPT_LANGUAGE_TAG,

                // Obsolete aliases kept to avoid breaking existing code.
                DWRITE_INFORMATIONAL_STRING_PREFERRED_FAMILY_NAMES = DWRITE_INFORMATIONAL_STRING_TYPOGRAPHIC_FAMILY_NAMES,
                DWRITE_INFORMATIONAL_STRING_PREFERRED_SUBFAMILY_NAMES = DWRITE_INFORMATIONAL_STRING_TYPOGRAPHIC_SUBFAMILY_NAMES,
                DWRITE_INFORMATIONAL_STRING_WWS_FAMILY_NAME = DWRITE_INFORMATIONAL_STRING_WEIGHT_STRETCH_STYLE_FAMILY_NAME,
            };


            internal enum DWRITE_FONT_STYLE
            {
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STYLE_OBLIQUE,
                DWRITE_FONT_STYLE_ITALIC
            }
            internal enum DWRITE_FONT_STRETCH
            {
                DWRITE_FONT_STRETCH_UNDEFINED = 0,
                DWRITE_FONT_STRETCH_ULTRA_CONDENSED = 1,
                DWRITE_FONT_STRETCH_EXTRA_CONDENSED = 2,
                DWRITE_FONT_STRETCH_CONDENSED = 3,
                DWRITE_FONT_STRETCH_SEMI_CONDENSED = 4,
                DWRITE_FONT_STRETCH_NORMAL = 5,
                DWRITE_FONT_STRETCH_MEDIUM = 5,
                DWRITE_FONT_STRETCH_SEMI_EXPANDED = 6,
                DWRITE_FONT_STRETCH_EXPANDED = 7,
                DWRITE_FONT_STRETCH_EXTRA_EXPANDED = 8,
                DWRITE_FONT_STRETCH_ULTRA_EXPANDED = 9
            }

            internal enum DWRITE_FONT_WEIGHT
            {
                DWRITE_FONT_WEIGHT_THIN = 100,
                DWRITE_FONT_WEIGHT_EXTRA_LIGHT = 200,
                DWRITE_FONT_WEIGHT_LIGHT = 300,
                DWRITE_FONT_WEIGHT_SEMI_LIGHT = 350,
                DWRITE_FONT_WEIGHT_NORMAL = 400,
                DWRITE_FONT_WEIGHT_MEDIUM = 500,
                DWRITE_FONT_WEIGHT_SEMI_BOLD = 600,
                DWRITE_FONT_WEIGHT_BOLD = 700,
                DWRITE_FONT_WEIGHT_EXTRA_BOLD = 800,
                DWRITE_FONT_WEIGHT_BLACK = 900,
                DWRITE_FONT_WEIGHT_EXTRA_BLACK = 950,
            }
            [Flags]
            internal enum DWRITE_FONT_SIMULATIONS
            {
                DWRITE_FONT_SIMULATIONS_NONE = 0x0000,
                DWRITE_FONT_SIMULATIONS_BOLD = 0x0001,
                DWRITE_FONT_SIMULATIONS_OBLIQUE = 0x0002
            }

            class Font
            {
                internal Font(IntPtr font)
                {
                    _font = font;
                }

                /// <summary>
                /// An entry in the _fontFaceCache array, maps Font to FontFace.
                /// </summary>
                internal struct FontFaceCacheEntry
                {
                    internal Font font;
                    internal FontFace fontFace;
                };

                /// <summary>
                /// The DWrite font object that this class wraps.
                /// </summary>
                IntPtr _font;

                /// <summary>
                /// The font's version number.
                /// </summary>
                double _version;

                /// <summary>
                /// FontMetrics for this font. Lazily allocated.
                /// </summary>
                FontMetrics _fontMetrics;

                /// <summary>
                /// Flags reflecting the state of the object.
                /// </summary>
                Flags _flags;

                /// <summary>
                /// Mutex used to control access to _fontFaceCache, which is locked when
                /// _mutex > 0.
                /// </summary>
                static int _mutex;

                /// <summary>
                /// Size of the _fontFaceCache, maximum number of FontFace instances cached.
                /// </summary>
                /// <remarks>
                /// Cache size could be based upon measurements of the TextFormatter micro benchmarks.
                /// English test cases allocate 1 - 3 FontFace instances, at the opposite extreme
                /// the Korean test maxes out at 13.  16 looks like a reasonable cache size.
                ///
                /// However, dwrite circa win7 has an issue aggressively consuming address space and
                /// therefore we need to be conservative holding on to font references.
                /// 

                const int _fontFaceCacheSize = 4;

                /// <summary>
                /// Cached FontFace instances.
                /// </summary>
                static FontFaceCacheEntry[] _fontFaceCache;

                /// <summary>
                /// Most recently used element in the FontFace cache.
                /// </summary>
                static int _fontFaceCacheMRU;

                [Flags]
                private enum Flags
                {
                    VersionInitialized = 0x0001,
                    IsSymbolFontInitialized = 0x0002,
                    IsSymbolFontValue = 0x0004,
                };

                public FontStyle Style
                {
                    get
                    {
                        DWRITE_FONT_STYLE dwriteFontStyle = NativeMethods.FontGetStyle(_font);
                        return DWriteTypeConverter.Convert(dwriteFontStyle);
                    }
                }
                public FontStretch Stretch
                {
                    get
                    {
                        DWRITE_FONT_STRETCH dwriteFontStretch = NativeMethods.FontGetStretch(_font);
                        return DWriteTypeConverter.Convert(dwriteFontStretch);
                    }
                }
                public FontWeight Weight
                {
                    get
                    {
                        DWRITE_FONT_WEIGHT dwriteFontWeight = NativeMethods.FontGetWeight(_font);
                        return DWriteTypeConverter.Convert(dwriteFontWeight);
                    }
                }

                public FontSimulations SimulationFlags
                {
                    get
                    {
                        DWRITE_FONT_SIMULATIONS dwriteFontSimulations = NativeMethods.FontGetSimulations(_font);
                        return DWriteTypeConverter.Convert(dwriteFontSimulations);
                    }
                }

                public LocalizedStrings FaceNames
                {
                    get
                    {
                        IntPtr dwriteLocalizedStrings;
                        NativeMethods.FontGetFaceNames(_font, out dwriteLocalizedStrings);
                        return new LocalizedStrings(dwriteLocalizedStrings);
                    }
                }

                public FontMetrics Metrics
                {
                    get 
                    {
                        if (_fontMetrics == null)
                        {
                            DWRITE_FONT_METRICS fontMetrics;
                            NativeMethods.FontGetMetrics(_font, out fontMetrics);
                            _fontMetrics = DWriteTypeConverter.Convert(fontMetrics);
                        }
                        return _fontMetrics;
                    }
                }

                public FontFace GetFontFace()
                {
                    FontFace fontFace = null;

                    if (Interlocked.Increment(ref _mutex) == 1)
                    {
                        if (_fontFaceCache != null)
                        {
                            FontFaceCacheEntry entry;
                            // Try the fast path first -- is caller accessing exactly the mru entry?
                            if ((entry = _fontFaceCache[_fontFaceCacheMRU]).font == this)
                            {
                                entry.fontFace.AddRef();
                                fontFace = entry.fontFace;
                            }
                            else
                            {
                                // No luck, do a search through the cache.
                                fontFace = LookupFontFaceSlow();
                            }
                        }
                    }
                    Interlocked.Decrement(ref _mutex);

                    // If the cache was busy or did not contain this Font, create a new FontFace.
                    if (null == fontFace)
                    {
                        fontFace = AddFontFaceToCache();
                    }

                    return fontFace;
                }

                /// <summary>
                /// Determines whether the font supports the specified character.
                /// </summary>
                /// <param name="unicodeValue">Unicode (UCS-4) character value.</param>
                /// <returns>TRUE if the font supports the specified character or FALSE if not.</returns>
                public bool HasCharacter(uint unicodeValue)
                {
                    bool exists = false;
                    NativeMethods.FontHasCharacter(_font, unicodeValue, out exists);
                    return exists;
                }

                FontFace CreateFontFace()
                {
                    IntPtr dwriteFontFace = IntPtr.Zero;
                    NativeMethods.FontCreateFontFace(_font, out dwriteFontFace);

                    return new FontFace(dwriteFontFace);
                }


                public bool IsSymbolFont
                {
                    get
                    {
                        if ((_flags & Flags.IsSymbolFontInitialized) != Flags.IsSymbolFontInitialized)
                        {
                            bool isSymbolFont = NativeMethods.FontIsSymbolFont(_font);
                            _flags |= Flags.IsSymbolFontInitialized;
                            if (isSymbolFont)
                            {
                                _flags |= Flags.IsSymbolFontValue;
                            }
                        }
                        return ((_flags & Flags.IsSymbolFontValue) == Flags.IsSymbolFontValue);
                    }
                }

                private FontFace AddFontFaceToCache()
                {
                    FontFace fontFace = CreateFontFace();
                    FontFace bumpedFontFace = null;

                    // NB: if the cache is busy, we simply return the new FontFace
                    // without bothering to cache it.
                    if (Interlocked.Increment(ref _mutex) == 1)
                    {
                        if (null == _fontFaceCache)
                        {
                            _fontFaceCache = new FontFaceCacheEntry[_fontFaceCacheSize];
                        }

                        // Default to a slot that is not the MRU.
                        _fontFaceCacheMRU = (_fontFaceCacheMRU + 1) % _fontFaceCacheSize;

                        // Look for an empty slot.
                        for (int i = 0; i < _fontFaceCacheSize; i++)
                        {
                            if (_fontFaceCache[i].font == null)
                            {
                                _fontFaceCacheMRU = i;
                                break;
                            }
                        }

                        // Keep a reference to any discarded entry, clean it up after releasing
                        // the mutex.
                        bumpedFontFace = _fontFaceCache[_fontFaceCacheMRU].fontFace;

                        // Record the new entry.
                        _fontFaceCache[_fontFaceCacheMRU].font = this;
                        _fontFaceCache[_fontFaceCacheMRU].fontFace = fontFace;
                        fontFace.AddRef();
                    }
                    Interlocked.Decrement(ref _mutex);

                    // If the cache was full and we replaced an unreferenced entry, release it now.
                    if (bumpedFontFace != null)
                    {
                        bumpedFontFace.Release();
                    }

                    return fontFace;
                }

                private FontFace LookupFontFaceSlow()
                {
                    FontFace fontFace = null;

                    for (int i = 0; i < _fontFaceCacheSize; i++)
                    {
                        if (_fontFaceCache[i].font == this)
                        {
                            fontFace = _fontFaceCache[i].fontFace;
                            fontFace.AddRef();
                            _fontFaceCacheMRU = i;
                            break;
                        }
                    }

                    return fontFace;
                }

                public static void ResetFontFaceCache()
                {
                    FontFaceCacheEntry[] fontFaceCache = null;

                    // NB: If the cache is busy, we do nothing.
                    if (Interlocked.Increment(ref _mutex) == 1)
                    {
                        fontFaceCache = _fontFaceCache;
                        _fontFaceCache = null;
                    }
                    Interlocked.Decrement(ref _mutex);

                    if (fontFaceCache != null)
                    {
                        for (int i = 0; i < _fontFaceCacheSize; i++)
                        {
                            if (fontFaceCache[i].fontFace != null)
                            {
                                fontFaceCache[i].fontFace.Release();
                            }
                        }
                    }
                }

                public bool GetInformationalStrings(
                    InformationalStringID informationalStringID,
                    out LocalizedStrings informationalStrings)
                {
                    IntPtr dwriteInformationalStrings;
                    bool exists = false;
                    NativeMethods.FontGetInformationalStrings(_font, DWriteTypeConverter.Convert(informationalStringID),
                                                         out dwriteInformationalStrings,
                                                          out exists);

                    informationalStrings = new LocalizedStrings(dwriteInformationalStrings);
                    return exists;

                }

                internal FontMetrics DisplayMetrics(float emSize, float pixelsPerDip)
                {
                    DWRITE_FONT_METRICS fontMetrics;
                    IntPtr fontFace = IntPtr.Zero;
                    NativeMethods.FontCreateFontFace(_font, out fontFace);
                    DWRITE_MATRIX transform = Factory.GetIdentityTransform();
                    NativeMethods.FontFaceGetGetGdiCompatibleMetrics(
                                                fontFace,
                                                emSize,
                                                pixelsPerDip,
                                                ref transform,
                                                out fontMetrics
                                                );
                    if (fontFace != IntPtr.Zero)
                    {
                        NativeMethods.FontFaceRelease(fontFace);
                        fontFace = IntPtr.Zero;
                    }

                    return DWriteTypeConverter.Convert(fontMetrics);
                }

                public IntPtr DWriteFontAddRef
                {
                    get
                    {
                        NativeMethods.FontAddRef(_font);
                        return _font;
                    }
                }

                public double Version
                {
                    get
                    {
                        if ((_flags & Flags.VersionInitialized) != Flags.VersionInitialized)
                        {
                            LocalizedStrings versionNumbers;
                            double version = 0.0;
                            if (GetInformationalStrings(InformationalStringID.VersionStrings, out versionNumbers))
                            {
                                String versionString = versionNumbers.GetString(0);

                                // The following logic assumes that the version string is always formatted in this way: "Version X.XX"
                                if (versionString.Length > 1)
                                {
                                    versionString = versionString.Substring(versionString.LastIndexOf(' ') + 1);
                                    if (!System.Double.TryParse(versionString, System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out version))
                                    {
                                        version = 0.0;
                                    }
                                }
                            }
                            _flags |= Flags.VersionInitialized;
                            _version = version;
                        }
                        return _version;
                    }
                }
            }

            class FontCollection
            {
                IntPtr _fontCollection;

                internal FontCollection(IntPtr fontCollection)
                {
                    _fontCollection = fontCollection;
                }

                public uint FamilyCount
                {
                    get
                    {
                        uint familyCount = NativeMethods.FontCollectionGetFontFamilyCount(_fontCollection);
                        return familyCount;
                    }
                }

                public FontFamily this[uint familyIndex]
                {
                    get
                    {
                        IntPtr dwriteFontFamily = IntPtr.Zero;

                        NativeMethods.FontCollectionGetFontFamily(_fontCollection,
                                                                   familyIndex,
                                                                   out dwriteFontFamily);

                        return new FontFamily(dwriteFontFamily);
                    }
                }

                public FontFamily this[string familyName]
                {
                    get
                    {
                        uint index;
                        bool exists = FindFamilyName(familyName,
                                                     out index);
                        if (exists)
                        {
                            return this[index];
                        }
                        return null;
                    }
                }

                private bool FindFamilyName(string familyName, out uint index)
                {
                    bool exists = false;
                    uint familyIndex = 0;
                    NativeMethods.FontCollectionFindFamilyName(_fontCollection,
                                                               familyName,
                                                               out familyIndex,
                                                               out exists);
                    index = familyIndex;
                    return exists;
                }
                public Font GetFontFromFontFace(FontFace fontFace)
                {
                    IntPtr dwriteFontFace = fontFace.DWriteFontFaceAddRef;
                    IntPtr dwriteFont = IntPtr.Zero;
                    int hr = NativeMethods.FontCollectionGetFontFromFontFace(
                                                                     _fontCollection,
                                                                     dwriteFontFace,
                                                                     out dwriteFont
                                                                     );

                    const int DWRITE_E_NOFONT = unchecked((int)0x88985002);

                    if (DWRITE_E_NOFONT == hr)
                    {
                        return null;
                    }
                    return new Font(dwriteFont);
                }
            }

            class FontList : IEnumerable<Font>
            {
                protected IntPtr _fontList;

                internal FontList(IntPtr fontList)
                {
                    _fontList = fontList;
                }

                internal uint Count
                {
                    get
                    {
                        return NativeMethods.FontListGetCount(_fontList);
                    }
                }

                internal Font this[uint index]
                {
                    get
                    {
                        IntPtr dwriteFont;
                        NativeMethods.FontListGetFont(_fontList, index, out dwriteFont);
                        return new Font(dwriteFont);
                    }
                }

                public IEnumerator<Font> GetEnumerator()
                {
                    return new FontsEnumerator(this);
                }

                IEnumerator IEnumerable.GetEnumerator()
                {
                    return new FontsEnumerator(this);
                }

                internal class FontsEnumerator : IEnumerator<Font>
                {
                    FontList _fontList;
                    int _currentIndex;

                    internal FontsEnumerator(FontList fontList)
                    {
                        _fontList = fontList;
                        _currentIndex = -1;
                    }

                    public virtual bool MoveNext()
                    {
                        if (_currentIndex >= _fontList.Count) //prevents _currentIndex from overflowing.
                        {
                            return false;
                        }
                        _currentIndex++;
                        return _currentIndex < _fontList.Count;
                    }

                    public Font Current
                    {
                        get
                        {
                            if (_currentIndex >= _fontList.Count)
                            {
                                throw new InvalidOperationException(LocalizedErrorMsgs.EnumeratorReachedEnd);
                            }
                            else if (_currentIndex == -1)
                            {
                                throw new InvalidOperationException(LocalizedErrorMsgs.EnumeratorNotStarted);
                            }
                            return _fontList[(uint)_currentIndex];

                        }
                    }

                    object IEnumerator.Current
                    {
                        get
                        {
                            return this.Current;
                        }
                    }

                    public virtual void Reset()
                    {
                        _currentIndex = -1;
                    }

                    public void Dispose()
                    {
                    }
                }
            }

            public class FontFace : IDisposable
            {
                private IntPtr _fontFace;
                private int _refCount;

                internal FontFace(IntPtr fontFace) { _fontFace = fontFace; }

                public void Dispose()
                {
                    if (_fontFace != IntPtr.Zero)
                    {
                        NativeMethods.FontFaceRelease(_fontFace);
                        _fontFace = IntPtr.Zero;
                    }
                }

                public void AddRef()
                {
                    Interlocked.Increment(ref _refCount);
                }

                public void Release()
                {
                    if (-1 == Interlocked.Decrement(ref _refCount))
                    {
                        // At this point we know the FontFace is eligable for the finalizer queue.
                        // However, because native dwrite font faces consume enormous amounts of 
                        // address space, we need to be aggressive about disposing immediately.
                        // If we rely solely on the GC finalizer, we will exhaust available address
                        // space in reasonable scenarios like enumerating all installed fonts.
                        Dispose();
                    }
                }

                public FontFaceType Type
                {
                    get
                    {
                        DWRITE_FONT_FACE_TYPE dwriteFontFaceType = NativeMethods.FontFaceGetType(_fontFace);
                        return DWriteTypeConverter.Convert(dwriteFontFaceType);
                    }
                }
                public uint Index
                {
                    get
                    {
                        uint index = NativeMethods.FontFaceGetIndex(_fontFace);
                        return index;
                    }
                }

                public ushort GlyphCount
                {
                    get
                    {
                        ushort glyphCount = NativeMethods.FontFaceGetGlyphCount(_fontFace);
                        return glyphCount;
                    }
                }
                public bool ReadFontEmbeddingRights(out ushort fsType)
                {
                    bool hasFontEmbeddingRights = NativeMethods.FontFaceReadFontEmbeddingRights(out fsType);
                    return hasFontEmbeddingRights;
                }

                public unsafe FontFile GetFileZero()
                {
                    uint numberOfFiles = 0;
                    IntPtr pfirstDWriteFontFile = IntPtr.Zero;
                    IntPtr[] ppDWriteFontFiles = null;

                    // This first call is to retrieve the numberOfFiles.
                    NativeMethods.FontFaceGetFiles(_fontFace,
                                                   out numberOfFiles,
                                                   null);

                    if (numberOfFiles > 0)
                    {
                        ppDWriteFontFiles = new IntPtr[numberOfFiles];
                        fixed(IntPtr* filesMem = ppDWriteFontFiles)
                        {
                            NativeMethods.FontFaceGetFiles(_fontFace,
                                                    out numberOfFiles,
                                                    filesMem
                                                    );
                        }

                        pfirstDWriteFontFile = ppDWriteFontFiles[0];

                        for (uint i = 1; i < numberOfFiles; ++i)
                        {
                            NativeMethods.FontFileRelease(ppDWriteFontFiles[i]);
                            ppDWriteFontFiles[i] = IntPtr.Zero;
                        }
                    }
                    return (numberOfFiles > 0) ? new FontFile(pfirstDWriteFontFile) : null;
                }

                public unsafe bool TryGetFontTable(OpenTypeTableTag openTypeTableTag, out byte[] tableData) 
                {
                    void* tableDataDWrite;
                    void* tableContext;
                    uint tableSizeDWrite = 0;
                    bool exists = false;
                    tableData = null;
                    int hr = NativeMethods.FontFaceTryGetFontTable(_fontFace,
                                                           (uint)openTypeTableTag,
                                                           out tableDataDWrite,
                                                           out tableSizeDWrite,
                                                           out tableContext,
                                                           out exists
                                                           );

                    if (exists)
                    {
                        tableData = new byte[tableSizeDWrite];
                        for (uint i = 0; i < tableSizeDWrite; ++i)
                        {
                            tableData[i] = ((byte*)tableDataDWrite)[i];
                        }

                        NativeMethods.FontFaceReleaseFontTable(_fontFace, tableContext);
                    }
                    return exists;
                }

                public IntPtr DWriteFontFaceAddRef
                {
                    get
                    {
                        NativeMethods.FontFaceAddRef(_fontFace);
                        return _fontFace;
                    }
                }

                public IntPtr DWriteFontFaceNoAddRef { get { return _fontFace; } }

                public unsafe void GetDisplayGlyphMetrics(
                    ushort* pGlyphIndices,
                    uint glyphCount,
                    MS.Internal.Text.TextInterface.GlyphMetrics* pGlyphMetrics,
                    double emSize,
                    bool useDisplayNatural,
                    bool isSideways,
                    float pixelsPerDip
                    )
                {
                    NativeMethods.FontFaceGetGdiCompatibleGlyphMetrics(_fontFace, emSize, pixelsPerDip, null,
                        useDisplayNatural, pGlyphIndices, glyphCount, pGlyphMetrics, isSideways);
                }

                public unsafe void GetDesignGlyphMetrics(
                    ushort* pGlyphIndices,
                    uint glyphCount,
                    MS.Internal.Text.TextInterface.GlyphMetrics* pGlyphMetrics
                    )
                {
                    NativeMethods.FontFaceGetDesignGlyphMetrics(_fontFace, pGlyphIndices, glyphCount, pGlyphMetrics);
                }


                public unsafe void GetArrayOfGlyphIndices(
                    uint* pCodePoints,
                    uint glyphCount,
                    ushort* pGlyphIndices
                    )
                {
                    NativeMethods.FontFaceGetGlyphIndices(_fontFace,
                                                        pCodePoints,
                                                        glyphCount,
                                                        pGlyphIndices
                                                        );
                }
            }

            public class FontFile : IDisposable
            {
                IntPtr _fontFile;

                public FontFile(IntPtr fontFile)
                {
                    _fontFile = fontFile;
                }

                public IntPtr DWriteFontFileNoAddRef { get { return _fontFile; } }

                public void Dispose()
                {
                    if (_fontFile != null)
                    {
                        NativeMethods.FontFileRelease(_fontFile);
                        _fontFile = IntPtr.Zero;
                    }
                }
                public unsafe string GetUriPath()
                {
                    uint pathLen = 0;
                    NativeMethods.FontFileGetUriPath(_fontFile, null, out pathLen);

                    char[] uriArray = new char[pathLen];

                    fixed (char* fixedUriArray = uriArray)
                    {
                        NativeMethods.FontFileGetUriPath(_fontFile, fixedUriArray, out pathLen);
                    }
                    Array.Resize(ref uriArray, uriArray.Length - 1);

                    return new string(uriArray);
                }

                internal bool Analyze(ref DWRITE_FONT_FILE_TYPE fontFileType, ref DWRITE_FONT_FACE_TYPE fontFaceType, ref uint numberOfFaces, out int hr)
                {
                    bool isSupported = false;
                    uint numberOfFacesDWrite = 0;
                    DWRITE_FONT_FILE_TYPE dwriteFontFileType;
                    DWRITE_FONT_FACE_TYPE dwriteFontFaceType;
                    hr = NativeMethods.FontFileAnalyze(
                                            _fontFile,
                                            out isSupported,
                                            out dwriteFontFileType,
                                            out dwriteFontFaceType,
                                            out numberOfFacesDWrite
                                            );

                    if (hr < 0)
                    {
                        return false;
                    }

                    fontFileType = dwriteFontFileType;
                    fontFaceType = dwriteFontFaceType;
                    numberOfFaces = numberOfFacesDWrite;
                    return isSupported;
                }
            }

            class FontFamily : FontList
            {
                internal FontFamily(IntPtr fontFamily) : base(fontFamily)
                {

                }

                Font _regularFont;

                public FontMetrics Metrics
                {
                    get
                    {
                        if (_regularFont == null)
                        {
                            _regularFont = GetFirstMatchingFont(FontWeight.Normal, FontStretch.Normal, FontStyle.Normal);
                        }
                        return _regularFont.Metrics;
                    }
                }
                public FontMetrics DisplayMetrics(float emSize, float pixelsPerDip)
                {
                    Font regularFont = GetFirstMatchingFont(FontWeight.Normal, FontStretch.Normal, FontStyle.Normal);
                    return regularFont.DisplayMetrics(emSize, pixelsPerDip);
                }

                public string OrdinalName
                {
                    get
                    {
                        if (FamilyNames.StringsCount > 0)
                        {
                            return FamilyNames.GetString(0);
                        }
                        return String.Empty;
                    }
                }

                public LocalizedStrings FamilyNames
                {
                    get
                    {
                        IntPtr dwriteLocalizedStrings;
                        NativeMethods.FontListGetFamilyNames(_fontList, out dwriteLocalizedStrings);
                        return new LocalizedStrings(dwriteLocalizedStrings);
                    }
                }

                public Font GetFirstMatchingFont(FontWeight weight, FontStretch stretch, FontStyle style)
                {
                    IntPtr dwriteFont;

                    NativeMethods.FontListGetFirstMatchingFont(_fontList,
                                                                DWriteTypeConverter.Convert(weight),
                                                                DWriteTypeConverter.Convert(stretch),
                                                                DWriteTypeConverter.Convert(style),
                                                                out dwriteFont);
                    return new Font(dwriteFont);
                }
            }

            class Factory
            {
                IntPtr _pFactory;
                FontCollectionLoader _wpfFontCollectionLoader;
                FontFileLoader _wpfFontFileLoader;
                IFontSourceFactory _fontSourceFactory;

                internal IntPtr DWriteFactoryAddRef
                {
                    get
                    {
                        NativeMethods.FactoryAddRef(_pFactory);
                        return _pFactory;
                    }
                }
                public static bool IsLocalUri(System.Uri uri) { return (uri.IsFile && uri.IsLoopback && !uri.IsUnc); }
                public FontCollection GetFontCollection(System.Uri uri)
                {
                    string uriString = uri.AbsoluteUri;
                    IntPtr dwriteFontCollection = IntPtr.Zero;

                    IntPtr pIDWriteFontCollectionLoaderMirror = Marshal.GetComInterfaceForObject(
                                        _wpfFontCollectionLoader,
                                        typeof(IDWriteFontCollectionLoaderMirror));

                    NativeMethods.FactoryCreateCustomFontCollection(
                                       _pFactory,
                                       pIDWriteFontCollectionLoaderMirror,
                                       uriString,
                                       (uriString.Length + 1) * sizeof(char),
                                       out dwriteFontCollection
                                       );

                    Marshal.Release(pIDWriteFontCollectionLoaderMirror);

                    return new FontCollection(dwriteFontCollection);
                }
                public FontCollection GetSystemFontCollection(bool checkForUpdates)
                {
                    IntPtr dwriteFontCollection = IntPtr.Zero;
                    NativeMethods.FactoryGetSystemFontCollection(_pFactory, out dwriteFontCollection, checkForUpdates);
                    return new FontCollection(dwriteFontCollection);
                }
                public FontCollection GetSystemFontCollection()
                {
                    return GetSystemFontCollection(false);
                }
                public static Factory Create(FactoryType factoryType, IFontSourceCollectionFactory fontSourceCollectionFactory, IFontSourceFactory fontSourceFactory)
                {
                    return new Factory(factoryType, fontSourceCollectionFactory, fontSourceFactory);
                }

                public Factory(FactoryType factoryType,
                               IFontSourceCollectionFactory fontSourceCollectionFactory,
                               IFontSourceFactory fontSourceFactory
                    )
                {
                    Initialize(factoryType);
                    _wpfFontFileLoader = new FontFileLoader(fontSourceFactory);
                    _wpfFontCollectionLoader = new FontCollectionLoader(fontSourceCollectionFactory,
                                                                    _wpfFontFileLoader);

                    _fontSourceFactory = fontSourceFactory;

                    IntPtr pIDWriteFontFileLoaderMirror = Marshal.GetComInterfaceForObject(
                                                            _wpfFontFileLoader,
                                                            typeof(IDWriteFontFileLoaderMirror));


                    NativeMethods.FactoryRegisterFontFileLoader(_pFactory, pIDWriteFontFileLoaderMirror);

                    Marshal.Release(pIDWriteFontFileLoaderMirror);

                    IntPtr pIDWriteFontCollectionLoaderMirror = Marshal.GetComInterfaceForObject(
                                        _wpfFontCollectionLoader,
                                        typeof(IDWriteFontCollectionLoaderMirror));

                    NativeMethods.FactoryRegisterFontCollectionLoader(_pFactory, pIDWriteFontCollectionLoaderMirror);

                    Marshal.Release(pIDWriteFontCollectionLoaderMirror);

                }

                private void Initialize(FactoryType factoryType)
                {
                    NativeMethods.FactoryCreate(
                        DWriteTypeConverter.Convert(factoryType),
                        out _pFactory
                        );
                }

                internal TextAnalyzer CreateTextAnalyzer()
                {
                    IntPtr textAnalyzer = IntPtr.Zero;
                    NativeMethods.FactoryCreateTextAnalyzer(_pFactory, out textAnalyzer);
                    return new TextAnalyzer(textAnalyzer);
                }

                internal FontFace CreateFontFace(Uri filePathUri, uint faceIndex, FontSimulations fontSimulationFlags)
                {
                    FontFile fontFile = CreateFontFile(filePathUri);
                    DWRITE_FONT_FILE_TYPE dwriteFontFileType = DWRITE_FONT_FILE_TYPE.DWRITE_FONT_FILE_TYPE_UNKNOWN;
                    DWRITE_FONT_FACE_TYPE dwriteFontFaceType = DWRITE_FONT_FACE_TYPE.DWRITE_FONT_FACE_TYPE_UNKNOWN;
                    uint numberOfFaces = 0;

                    int hr = 0;
                    if (fontFile.Analyze(
                                         ref dwriteFontFileType,
                                         ref dwriteFontFaceType,
                                         ref numberOfFaces,
                                         out hr
                                         ))
                    {
                        if (faceIndex >= numberOfFaces)
                        {
                            throw new ArgumentOutOfRangeException("faceIndex");
                        }

                        DWRITE_FONT_SIMULATIONS dwriteFontSimulationsFlags = DWriteTypeConverter.Convert(fontSimulationFlags);
                        IntPtr dwriteFontFace = IntPtr.Zero;
                        IntPtr dwriteFontFile = fontFile.DWriteFontFileNoAddRef;

                        NativeMethods.FactoryCreateFontFace(_pFactory,
                                                             dwriteFontFaceType,
                                                             1,
                                                             dwriteFontFile,
                                                             faceIndex,
                                                             (DWRITE_FONT_SIMULATIONS)dwriteFontSimulationsFlags,
                                                             out dwriteFontFace
                                                             );
                        return new FontFace(dwriteFontFace);
                    }
                    else
                    {
                        const int DWRITE_E_FILEFORMAT = unchecked((int)0x88985000);

                        if (hr == DWRITE_E_FILEFORMAT)
                        {
                            IFontSource fontSource = _fontSourceFactory.Create(filePathUri.AbsoluteUri);
                            fontSource.TestFileOpenable();
                        }
                    }

                    return null;
                }

                private FontFile CreateFontFile(Uri filePathUri)
                {
                    IntPtr dwriteFontFile = IntPtr.Zero;
                    Factory.CreateFontFile(_pFactory, _wpfFontFileLoader, filePathUri, out dwriteFontFile);

                    return new FontFile(dwriteFontFile);
                }

                internal static unsafe int CreateFontFile(IntPtr pFactory, FontFileLoader fontFileLoader, Uri filePathUri, out IntPtr dwriteFontFile)
                {
                    bool isLocal = IsLocalUri(filePathUri);

                    if (isLocal)
                    {
                        NativeMethods.FactoryCreateFontFileReference(pFactory,
                                                              filePathUri.LocalPath,
                                                              null,
                                                              out dwriteFontFile
                                                              );
                    }
                    else
                    {
                        string filePath = filePathUri.AbsoluteUri;

                        IntPtr pIDWriteFontLoaderMirror = Marshal.GetComInterfaceForObject(
                                                                fontFileLoader,
                                                                typeof(IDWriteFontFileLoaderMirror));

                        NativeMethods.FactoryCreateCustomFontFileReference(pFactory,
                                                                    filePath,
                                                                    (filePath.Length + 1) * sizeof(char),
                                                                    pIDWriteFontLoaderMirror,
                                                                    out dwriteFontFile
                                                                    );

                        Marshal.Release(pIDWriteFontLoaderMirror);
                    }

                    return HRESULT.S_OK;  
                }

                internal static DWRITE_MATRIX GetIdentityTransform()
                {
                    DWRITE_MATRIX transform = new DWRITE_MATRIX();
                    transform.m11 = 1;
                    transform.m12 = 0;
                    transform.m22 = 1;
                    transform.m21 = 0;
                    transform.dx = 0;
                    transform.dy = 0;

                    return transform;
                }
            }

            class LocalizedStrings : IDictionary<CultureInfo, string>
            {
                IntPtr _localizedStrings;
                CultureInfo[] _keys;
                String[] _values;

                internal LocalizedStrings(IntPtr localizedStrings) { _localizedStrings = localizedStrings; }
                internal LocalizedStrings() { _localizedStrings = IntPtr.Zero; }

                string IDictionary<CultureInfo, string>.this[CultureInfo key]
                {
                    get
                    {
                        String value;
                        if (TryGetValue(key, out value))
                        {
                            return value;
                        }
                        else
                        {
                            return null;
                        }
                    }
                    set
                    {
                        throw new NotSupportedException();
                    }
                }

                private bool TryGetValue(CultureInfo key, out string value)
                {
                    Invariant.Assert(key != null);
                    value = null;

                    uint index = 0;
                    if (FindLocaleName(key.Name, out index))
                    {
                        value = GetString((int)index);
                        return true;
                    }
                    return false;
                }

                private bool FindLocaleName(string name, out uint index)
                {
                    if (_localizedStrings == null)
                    {
                        index = 0;
                        return false;
                    }
                    else
                    {
                        bool exists = false;
                        uint localeNameIndex = 0;
                        NativeMethods.LocalizedStringsFindLocaleName(_localizedStrings,
                                                                      name,
                                                                      out localeNameIndex,
                                                                      out exists
                                                                      );
                        index = localeNameIndex;
                        return exists;
                    }
                }

                ICollection<CultureInfo> IDictionary<CultureInfo, string>.Keys
                {
                    get
                    {
                        return KeysArray;
                    }
                }

                internal CultureInfo[] KeysArray
                {
                    get
                    {
                        if (_keys == null)
                        {
                            _keys = new CultureInfo[StringsCount];
                            for (uint i = 0; i < StringsCount; i++)
                            {
                                _keys[i] = new CultureInfo(GetLocaleName(i));
                            }
                        }
                        return _keys;
                    }
                }

                private unsafe string GetLocaleName(uint index)
                {
                    if (_localizedStrings == null)
                    {
                        return string.Empty;
                    }
                    else
                    {
                        uint stringLength = GetLocaleNameLength(index);
                        Invariant.Assert(stringLength >= 0 && stringLength < uint.MaxValue);
                        char[] stringBuffer = new char[stringLength + 1];

                        fixed (char* stringWCHAR = &stringBuffer[0])
                        {
                            NativeMethods.LocalizedStringsGetLocaleName(_localizedStrings,
                                                                    index,
                                                                    stringWCHAR,
                                                                    stringLength + 1);
                            string str = new string(stringWCHAR);
                            return str;
                        }
                    }
                }

                private uint GetLocaleNameLength(uint index)
                {
                    if (_localizedStrings == IntPtr.Zero)
                    {
                        return 0;
                    }
                    else
                    {
                        uint length = 0;
                        NativeMethods.LocalizedStringsGetLocaleNameLength(_localizedStrings, index, out length);
                        return length;
                    }
                }

                ICollection<string> IDictionary<CultureInfo, string>.Values
                {
                    get
                    {
                        return ValuesArray;
                    }
                }

                internal String[] ValuesArray
                {
                    get
                    {
                        if (_values == null)
                        {
                            _values = new string[StringsCount];

                            for (uint i = 0; i < StringsCount; i++)
                            {
                                _values[i] = GetString((int)i);
                            }
                        }

                        return _values;
                    }
                }
                int ICollection<KeyValuePair<CultureInfo, string>>.Count => throw new NotImplementedException();

                bool ICollection<KeyValuePair<CultureInfo, string>>.IsReadOnly
                {
                    get
                    {
                        return true;
                    }
                }

                public uint StringsCount
                {
                    get
                    {
                        uint count = (_localizedStrings != IntPtr.Zero) ? NativeMethods.LocalizedStringsGetCount(_localizedStrings) : 0;
                        return count;
                    }
                }

                void IDictionary<CultureInfo, string>.Add(CultureInfo key, string value)
                {
                    throw new NotSupportedException();
                }

                void ICollection<KeyValuePair<CultureInfo, string>>.Add(KeyValuePair<CultureInfo, string> item)
                {
                    throw new NotSupportedException();
                }

                void ICollection<KeyValuePair<CultureInfo, string>>.Clear()
                {
                    throw new NotSupportedException();
                }

                bool ICollection<KeyValuePair<CultureInfo, string>>.Contains(KeyValuePair<CultureInfo, string> item)
                {
                    throw new NotImplementedException();
                }

                bool IDictionary<CultureInfo, string>.ContainsKey(CultureInfo key)
                {
                    uint index = 0;
                    return FindLocaleName(key.Name, out index);
                }

                void ICollection<KeyValuePair<CultureInfo, string>>.CopyTo(KeyValuePair<CultureInfo, string>[] array, int arrayIndex)
                {
                    int index = arrayIndex;
                    foreach (KeyValuePair<CultureInfo, String> pair in this)
                    {
                        array[index] = pair;
                        ++index;
                    }
                }

                IEnumerator<KeyValuePair<CultureInfo, string>> IEnumerable<KeyValuePair<CultureInfo, string>>.GetEnumerator()
                {
                    return new LocalizedStringsEnumerator(this);
                }

                IEnumerator IEnumerable.GetEnumerator()
                {
                    return new LocalizedStringsEnumerator(this);
                }

                bool IDictionary<CultureInfo, string>.Remove(CultureInfo key)
                {
                    throw new NotSupportedException();
                }

                bool ICollection<KeyValuePair<CultureInfo, string>>.Remove(KeyValuePair<CultureInfo, string> item)
                {
                    throw new NotSupportedException();
                }

                bool IDictionary<CultureInfo, string>.TryGetValue(CultureInfo key, out string value)
                {
                    Invariant.Assert(key != null);
                    value = null;

                    uint index = 0;
                    if (FindLocaleName(key.Name, out index))
                    {
                        value = GetString((int)index);
                        return true;
                    }
                    return false;
                }

                internal unsafe string GetString(int index)
                {
                    if (_localizedStrings == null)
                    {
                        return string.Empty;
                    }
                    else
                    {
                        uint stringLength = GetStringLength(index);
                        Invariant.Assert(stringLength >= 0 && stringLength < uint.MaxValue);
                        char[] stringBuffer = new char[stringLength + 1];

                        fixed (char* stringWCHAR = &stringBuffer[0])
                        {
                            NativeMethods.LocalizedStringsGetString(_localizedStrings,
                                                                    index,
                                                                    stringWCHAR,
                                                                    stringLength + 1);
                            string str = new string(stringWCHAR);
                            return str;
                        }
                    }
                }

                private uint GetStringLength(int index)
                {
                    if (_localizedStrings == IntPtr.Zero)
                    {
                        return 0;
                    }
                    else
                    {
                        uint length = 0;
                        NativeMethods.LocalizedStringsGetStringLength(_localizedStrings, index, out length);
                        return length;
                    }
                }
            }

            class LocalizedStringsEnumerator : IEnumerator<KeyValuePair<CultureInfo, String>>
            {
                LocalizedStrings _localizedStrings;
                long _currentIndex;
                internal LocalizedStringsEnumerator(LocalizedStrings localizedStrings)
                {
                    _localizedStrings = localizedStrings;
                    _currentIndex = -1;
                }

                public virtual bool MoveNext()
                {
                    if (_currentIndex >= _localizedStrings.StringsCount) //prevents _currentIndex from overflowing.
                    {
                        return false;
                    }
                    _currentIndex++;
                    return _currentIndex < _localizedStrings.StringsCount;
                }

                public KeyValuePair<CultureInfo, String> Current
                {
                    get
                    {
                        if (_currentIndex >= _localizedStrings.StringsCount)
                        {
                            throw new System.InvalidOperationException(LocalizedErrorMsgs.EnumeratorReachedEnd);
                        }
                        else if (_currentIndex == -1)
                        {
                            throw new System.InvalidOperationException(LocalizedErrorMsgs.EnumeratorNotStarted);
                        }

                        CultureInfo[] keys = _localizedStrings.KeysArray;
                        String[] values = _localizedStrings.ValuesArray;
                        KeyValuePair<CultureInfo, String> current = new KeyValuePair<CultureInfo, String>(keys[(uint)_currentIndex], values[(uint)_currentIndex]);
                        return current;
                    }
                }

                Object IEnumerator.Current
                {
                    get
                    {
                        return this.Current;
                    }
                }

                public virtual void Reset()
                {
                    _currentIndex = -1;
                }

                public void Dispose()
                {
                }
            }

            internal struct DWRITE_SCRIPT_ANALYSIS
            {
                internal ushort script;
                internal int shapes;
            }

            [ClassInterface(ClassInterfaceType.None), ComVisible(true)]
            internal class FontCollectionLoader : IDWriteFontCollectionLoaderMirror
            {
                private IFontSourceCollectionFactory _fontSourceCollectionFactory;
                private FontFileLoader _fontFileLoader;

                public FontCollectionLoader(IFontSourceCollectionFactory fontSourceCollectionFactory, FontFileLoader fontFileLoader)
                {
                    _fontSourceCollectionFactory = fontSourceCollectionFactory;
                    _fontFileLoader = fontFileLoader;
                }

                public unsafe int CreateEnumeratorFromKey(IntPtr factory, [In] void* collectionKey, [In, MarshalAs(UnmanagedType.U4)] uint collectionKeySize, IntPtr* fontFileEnumerator)
                {
                    uint numberOfCharacters = collectionKeySize / sizeof(char);
                    if ((fontFileEnumerator == null)
                        || (collectionKeySize % sizeof(char) != 0)                      // The collectionKeySize must be divisible by sizeof(WCHAR)
                        || (numberOfCharacters <= 1)                                            // The collectionKey cannot be less than or equal 1 character as it has to contain the NULL character.
                        || (((char*)collectionKey)[numberOfCharacters - 1] != '\0'))    // The collectionKey must end with the NULL character
                    {
                        return HRESULT.E_FAIL;
                    }
                    *fontFileEnumerator = IntPtr.Zero;

                    String uriString = new String((char*)collectionKey);
                    int hr = HRESULT.S_OK;

                    try
                    {
                        IFontSourceCollection fontSourceCollection = _fontSourceCollectionFactory.Create(uriString);
                        FontFileEnumerator fontFileEnum = new FontFileEnumerator(
                                                                   fontSourceCollection,
                                                                   _fontFileLoader,
                                                                   factory);
                        IntPtr pIDWriteFontFileEnumeratorMirror = Marshal.GetComInterfaceForObject(
                                                                fontFileEnum,
                                                                typeof(IDWriteFontFileEnumeratorMirror));

                        *fontFileEnumerator = pIDWriteFontFileEnumeratorMirror;
                    }
                    catch (System.Exception exception)
                    {
                        hr = System.Runtime.InteropServices.Marshal.GetHRForException(exception);
                    }

                    return hr;
                }
            }

            class FontFileEnumerator : IDWriteFontFileEnumeratorMirror
            {
                IEnumerator<IFontSource> _fontSourceCollectionEnumerator;
                FontFileLoader _fontFileLoader;
                IntPtr _factory;

                FontFileEnumerator() { }

                internal FontFileEnumerator(
                                  IEnumerable<IFontSource> fontSourceCollection,
                                  FontFileLoader fontFileLoader,
                                  IntPtr factory
                                  )
                {
                    _fontSourceCollectionEnumerator = fontSourceCollection.GetEnumerator();
                    _fontFileLoader = fontFileLoader;
                    NativeMethods.FactoryAddRef(factory);
                    _factory = factory;
                }

                /// <summary>
                /// Advances to the next font file in the collection. When it is first created, the enumerator is positioned
                /// before the first element of the collection and the first call to MoveNext advances to the first file.
                /// </summary>
                /// <param name="hasCurrentFile">Receives the value TRUE if the enumerator advances to a file, or FALSE if
                /// the enumerator advanced past the last file in the collection.</param>
                /// <returns>
                /// Standard HRESULT error code.
                /// </returns>
                public int MoveNext(
                                   out bool hasCurrentFile
                                   )
                {
                    hasCurrentFile = false;

                    int hr = 0;
                    try
                    {
                        hasCurrentFile = _fontSourceCollectionEnumerator.MoveNext();
                    }
                    catch (System.Exception exception)
                    {
                        hr = System.Runtime.InteropServices.Marshal.GetHRForException(exception);
                    }

                    return hr;
                }

                /// <summary>
                /// Gets a reference to the current font file.
                /// </summary>
                /// <param name="fontFile">Pointer to the newly created font file object.</param>
                /// <returns>
                /// Standard HRESULT error code.
                /// </returns>
                [ComVisible(true)]
                public int GetCurrentFontFile(
                                             out IntPtr fontFile
                                             )
                {
                    return Factory.CreateFontFile(
                                                  _factory,
                                                  _fontFileLoader,
                                                  _fontSourceCollectionEnumerator.Current.Uri,
                                                  out fontFile
                                                  );
                }
            }
        }

    }
}


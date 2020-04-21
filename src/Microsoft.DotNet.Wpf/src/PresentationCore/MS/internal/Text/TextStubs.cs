using System;
using System.IO;
using System.Threading;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Windows.Media;
using MS.Internal.FontCache;
using MS.Internal.Text.TextInterface;
using MS.Win32;
using MS.Internal.Ink;
using MS.Internal.Interop;

namespace MS.Internal
{
    public class NativeWPFDLLLoader
    {
        internal static void LoadDwrite()
        {
            // No implementation needed?
        }
    }

    public class TrueTypeSubsetter
    {
        internal static unsafe byte[] ComputeSubset(void* fontData, int fileSize, Uri sourceUri, int directoryOffset, ushort[] glyphArray)
        {
            const int NO_ERROR = 0;
            const int ERR_WOULD_GROW = 1007;

            byte* puchDestBuffer = null;
            ulong ulDestBufferSize = 0, ulBytesWritten = 0;

            Invariant.Assert(glyphArray != null && glyphArray.Length > 0 && glyphArray.Length <= ushort.MaxValue);

            fixed(ushort* pinnedGlyphArray = &glyphArray[0])
            {
                short errCode = MS.Internal.Text.TextInterface.NativeMethods.TrueTypeSubsetCreateDeltaTTF(
                    (byte*)fontData,
                    fileSize,
                    &puchDestBuffer,
                    &ulDestBufferSize,
                    &ulBytesWritten,
                    0, // format of the subset font to create. 0 = Subset
                    0, // all languages in the Name table should be retained
                    0, // Ignored for usListType = 1
                    0, // Ignored for usListType = 1
                    1, // usListType, 1 means the KeepCharCodeList represents raw Glyph indices from the font
                    pinnedGlyphArray, // glyph indices array
                    (ushort)glyphArray.Length, // number of glyph indices
                    directoryOffset);

                byte[] retArray = null;

                try
                {
                    if (errCode == NO_ERROR)
                    {
                        retArray = new byte[ulBytesWritten];
                        System.Runtime.InteropServices.Marshal.Copy((System.IntPtr)puchDestBuffer, retArray, 0, (int)ulBytesWritten);
                    }
                }
                finally
                {
                    MS.Internal.Text.TextInterface.NativeMethods.TrueTypeSubsetFreeMem(puchDestBuffer);
                }

                // If subsetting would grow the font, just use the original one as it's the best we can do.
                if (errCode == ERR_WOULD_GROW)
                {
                    retArray = new byte[fileSize];
                    System.Runtime.InteropServices.Marshal.Copy((System.IntPtr)fontData, retArray, 0, fileSize);
                }
                else if (errCode != NO_ERROR)
                {
                    throw new FileFormatException(sourceUri);
                }

                return retArray;
            }
        }
    }

    namespace Text
    {
        namespace TextInterface
        {
            public struct FILETIME
            {
                public uint DateTimeLow;
                public uint DateTimeHigh;
            }

            public class NativeMethods
            {
                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern DWRITE_FONT_STYLE FontGetStyle(IntPtr font);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern DWRITE_FONT_STRETCH FontGetStretch(IntPtr font);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern DWRITE_FONT_WEIGHT FontGetWeight(IntPtr font);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern DWRITE_FONT_SIMULATIONS FontGetSimulations(IntPtr font);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FontGetFaceNames(IntPtr font, out IntPtr dwriteLocalizedStrings);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FontHasCharacter(IntPtr font, uint unicodeValue, out bool exists);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FontCreateFontFace(IntPtr font, out IntPtr dwriteFontFace);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern bool FontIsSymbolFont(IntPtr font);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FontGetInformationalStrings(IntPtr font, DWRITE_INFORMATIONAL_STRING_ID dWRITE_INFORMATIONAL_STRING_ID, out IntPtr dwriteInformationalStrings, out bool exists);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FontFaceGetGetGdiCompatibleMetrics(IntPtr fontFace, float emSize, float pixelsPerDip, ref DWRITE_MATRIX transform, out DWRITE_FONT_METRICS fontMetrics);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FontFaceRelease(IntPtr fontFace);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FontAddRef(IntPtr font);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern uint FontCollectionGetFontFamilyCount(IntPtr fontCollection);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FontCollectionGetFontFamily(IntPtr fontCollection, uint familyIndex, out IntPtr dwriteFontFamily);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FontCollectionFindFamilyName(IntPtr fontCollection, string familyName, out uint familyIndex, out bool exists);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern int FontCollectionGetFontFromFontFace(IntPtr fontCollection, IntPtr dwriteFontFace, out IntPtr dwriteFont);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern uint FontListGetCount(IntPtr fontList);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FontListGetFont(IntPtr fontList, uint index, out IntPtr dwriteFont);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern DWRITE_FONT_FACE_TYPE FontFaceGetType(IntPtr fontFace);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern uint FontFaceGetIndex(IntPtr fontFace);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern ushort FontFaceGetGlyphCount(IntPtr fontFace);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern bool FontFaceReadFontEmbeddingRights(out ushort fsType);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern unsafe void FontFaceGetFiles(IntPtr fontFace, out uint numberOfFiles, void* filesMem);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FontFileRelease(IntPtr intPtr);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FontFaceAddRef(IntPtr fontFace);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static unsafe extern void FontFaceGetGdiCompatibleGlyphMetrics(IntPtr fontFace, double emSize, float pixelsPerDip, DWRITE_MATRIX* matrix, bool useDisplayNatural, ushort* pGlyphIndices, uint glyphCount, GlyphMetrics* pGlyphMetrics, bool isSideways);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static unsafe extern void FontFaceGetDesignGlyphMetrics(IntPtr fontFace, ushort* pGlyphIndices, uint glyphCount, GlyphMetrics* pGlyphMetrics);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static unsafe extern void FontFaceGetGlyphIndices(IntPtr fontFace, uint* pCodePoints, uint glyphCount, ushort* pGlyphIndices);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static unsafe extern void FontFileGetUriPath(IntPtr fontFile, char* buffer, out uint pathLen);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern int FontFileAnalyze(IntPtr fontFile, out bool isSupported, out DWRITE_FONT_FILE_TYPE dwriteFontFileType, out DWRITE_FONT_FACE_TYPE dwriteFontFaceType, out uint numberOfFacesDWrite);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FontListGetFamilyNames(IntPtr fontList, out IntPtr dwriteLocalizedStrings);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FontListGetFirstMatchingFont(IntPtr fontList, DWRITE_FONT_WEIGHT dWRITE_FONT_WEIGHT, DWRITE_FONT_STRETCH dWRITE_FONT_STRETCH, DWRITE_FONT_STYLE dWRITE_FONT_STYLE, out IntPtr dwriteFont);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FactoryCreateCustomFontCollection(IntPtr pFactory, IntPtr fontCollectionLoader, string uriString, int collectionKeySize, out IntPtr dwriteFontCollection);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FactoryGetSystemFontCollection(IntPtr pFactory, out IntPtr dwriteFontCollection, bool checkForUpdates);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FactoryRegisterFontFileLoader(IntPtr pFactory, IntPtr pIDWriteFontFileLoaderMirror);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FactoryRegisterFontCollectionLoader(IntPtr pFactory, IntPtr pIDWriteFontCollectionLoaderMirror);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FactoryCreate(DWRITE_FACTORY_TYPE dWRITE_FACTORY_TYPE, out IntPtr pFactory);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FactoryCreateTextAnalyzer(IntPtr pFactory, out IntPtr textAnalyzer);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FactoryCreateFontFace(IntPtr pFactory, DWRITE_FONT_FACE_TYPE dwriteFontFaceType, int v, IntPtr dwriteFontFile, uint faceIndex, DWRITE_FONT_SIMULATIONS dwriteFontSimulationsFlags, out IntPtr dwriteFontFace);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern unsafe void FactoryCreateFontFileReference(IntPtr pFactory, string localPath, FILETIME* pFileTime, out IntPtr dwriteFontFile);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FactoryCreateCustomFontFileReference(IntPtr pFactory, string filePath, int len, IntPtr fontFileLoader, out IntPtr dwriteFontFile);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void LocalizedStringsFindLocaleName(IntPtr localizedStrings, string name, out uint localeNameIndex, out bool exists);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static unsafe extern void LocalizedStringsGetLocaleName(IntPtr localizedStrings, uint index, char* stringWCHAR, uint v);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void LocalizedStringsGetLocaleNameLength(IntPtr localizedStrings, uint index, out uint length);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern uint LocalizedStringsGetCount(IntPtr localizedStrings);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static unsafe extern void LocalizedStringsGetString(IntPtr localizedStrings, int index, char* stringWCHAR, uint v);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void LocalizedStringsGetStringLength(IntPtr localizedStrings, int index, out uint length);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FactoryAddRef(IntPtr factory);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static extern void FontGetMetrics(IntPtr font, out DWRITE_FONT_METRICS fontMetrics);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static unsafe extern int FontFaceTryGetFontTable(IntPtr pFontFace, uint openTypeTableTag, out void* tableDataDWrite, out uint tableSizeDWrite, out void* tableContext, out bool exists);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static unsafe extern void FontFaceReleaseFontTable(IntPtr pFontFace, void* tableContext);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static unsafe extern int TextAnalyzerAnalyzeScript(IntPtr pTextAnalyzer, void* pTextAnalysisSource, int v, uint length, void* pTextAnalysisSink);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static unsafe extern int TextAnalyzerAnalyzeNumberSubstitution(IntPtr pTextAnalyzer, void* pTextAnalysisSource, int v, uint length, void* pTextAnalysisSink);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static unsafe extern int TextAnalyzerGetGdiCompatibleGlyphPlacements(IntPtr textAnalyzer, char* textString, ushort* clusterMap, ushort* textProps, uint textLength, ushort* glyphIndices, uint* glyphProps, uint glyphCount, IntPtr dWriteFontFaceNoAddRef, float fontEmSizeFloat, float pixelsPerDip, DWRITE_MATRIX* dWRITE_MATRIX, bool v, bool isSideways, bool isRightToLeft, DWRITE_SCRIPT_ANALYSIS* scriptAnalysis, string locale, DWRITE_TYPOGRAPHIC_FEATURES** dwriteTypographicFeatures, uint* pFeatureRangeLengths, uint featureRanges, float[] dwriteGlyphAdvances, DWRITE_GLYPH_OFFSET[] dwriteGlyphOffsets);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static unsafe extern int TextAnalyzerGetGlyphPlacements(IntPtr textAnalyzer, char* textString, ushort* clusterMap, ushort* textProps, uint textLength, ushort* glyphIndices, uint* glyphProps, uint glyphCount, IntPtr dWriteFontFaceNoAddRef, float fontEmSizeFloat, bool isSideways, bool isRightToLeft, DWRITE_SCRIPT_ANALYSIS* scriptAnalysis, string localeName, DWRITE_TYPOGRAPHIC_FEATURES** dwriteTypographicFeatures, uint* pFeatureRangeLengths, uint featureRanges, float[] dwriteGlyphAdvances, DWRITE_GLYPH_OFFSET[] dwriteGlyphOffsets);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static unsafe extern int TextAnalyzerGetGlyphs(IntPtr textAnalyzer, char* textString, uint textLength, IntPtr dWriteFontFaceNoAddRef, bool isSideways, bool isRightToLeft, DWRITE_SCRIPT_ANALYSIS* scriptAnalysis, string localeName, IntPtr numberSubstitutionNoAddRef, DWRITE_TYPOGRAPHIC_FEATURES** dwriteTypographicFeatures, uint[] featureRangeLengths, uint featureRanges, uint maxGlyphCount, ushort* clusterMap, ushort* textProps, ushort* glyphIndices, uint* glyphProps, out uint glyphCount);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static unsafe extern short TrueTypeSubsetCreateDeltaTTF(byte* fontData, int fileSize, byte** v1, ulong* v2, ulong* v3, int v4, int v5, int v6, int v7, int v8, ushort* pinnedGlyphArray, ushort length, int directoryOffset);

                [DllImport("DWriteNativeMethods.dll", CharSet = CharSet.Auto, SetLastError = true)]
                internal static unsafe extern void TrueTypeSubsetFreeMem(byte* puchDestBuffer);
            }

            /// <summary>
            /// The font file enumerator interface encapsulates a collection of font files. The font system uses this interface
            /// to enumerate font files when building a font collection.
            /// </summary>
            [ComImport(), Guid("72755049-5ff7-435d-8348-4be97cfa6c7c"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
            interface IDWriteFontFileEnumeratorMirror
            {
                /// <summary>
                /// Advances to the next font file in the collection. When it is first created, the enumerator is positioned
                /// before the first element of the collection and the first call to MoveNext advances to the first file.
                /// </summary>
                /// <param name="hasCurrentFile">Receives the value TRUE if the enumerator advances to a file, or FALSE if
                /// the enumerator advanced past the last file in the collection.</param>
                /// <returns>
                /// Standard HRESULT error code.
                /// </returns>
                [PreserveSig]
                int MoveNext(
                    [Out, MarshalAs(UnmanagedType.Bool)] out bool hasCurrentFile

                    );

                /// <summary>
                /// Gets a reference to the current font file.
                /// </summary>
                /// <param name="fontFile">Pointer to the newly created font file object.</param>
                /// <returns>
                /// Standard HRESULT error code.
                /// </returns>
                [PreserveSig]
                int GetCurrentFontFile(
                    /*[Out, MarshalAs(UnmanagedType::Interface)]*/ out IntPtr fontFile
                    );
            };

            /// <summary>
            /// The font collection loader interface is used to construct a collection of fonts given a particular type of key.
            /// The font collection loader interface is recommended to be implemented by a singleton object.
            /// IMPORTANT: font collection loader implementations must not register themselves with DirectWrite factory
            /// inside their constructors and must not unregister themselves in their destructors, because
            /// registration and unregistraton operations increment and decrement the object reference count respectively.
            /// Instead, registration and unregistration of font file loaders with DirectWrite factory should be performed
            /// outside of the font file loader implementation as a separate step.
            /// </summary>
            [ComImport(), Guid("cca920e4-52f0-492b-bfa8-29c72ee0a468"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
            interface IDWriteFontCollectionLoaderMirror
            {
                /// <summary>
                /// Creates a font file enumerator object that encapsulates a collection of font files.
                /// The font system calls back to this interface to create a font collection.
                /// </summary>
                /// <param name="collectionKey">Font collection key that uniquely identifies the collection of font files within
                /// the scope of the font collection loader being used.</param>
                /// <param name="collectionKeySize">Size of the font collection key in bytes.</param>
                /// <param name="fontFileEnumerator">Pointer to the newly created font file enumerator.</param>
                /// <returns>
                /// Standard HRESULT error code.
                /// </returns>
                [PreserveSig]
                unsafe int CreateEnumeratorFromKey(
                    /*[In, MarshalAs(UnmanagedType::Interface)]*/ IntPtr factory,
                    [In] void* collectionKey,
                    [In, MarshalAs(UnmanagedType.U4)] uint collectionKeySize,
                    /*[Out, MarshalAs(UnmanagedType::Interface)]*/ IntPtr* fontFileEnumerator
                    );
            };

            /// <summary>
            /// The interface for loading font file data.
            /// </summary>
            [ComImport(), Guid("6d4865fe-0ab8-4d91-8f62-5dd6be34a3e0"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
            interface IDWriteFontFileStreamMirror
            {
                /// <summary>
                /// Reads a fragment from a file.
                /// </summary>
                /// <param name="fragmentStart">Receives the pointer to the start of the font file fragment.</param>
                /// <param name="fileOffset">Offset of the fragment from the beginning of the font file.</param>
                /// <param name="fragmentSize">Size of the fragment in bytes.</param>
                /// <param name="fragmentContext">The client defined context to be passed to the ReleaseFileFragment.</param>
                /// <returns>
                /// Standard HRESULT error code.
                /// </returns>
                /// <remarks>
                /// IMPORTANT: ReadFileFragment() implementations must check whether the requested file fragment
                /// is within the file bounds. Otherwise, an error should be returned from ReadFileFragment.
                /// </remarks>
                [PreserveSig]
                unsafe int ReadFileFragment(
                    void** fragmentStart,
                    [In, MarshalAs(UnmanagedType.U8)] ulong fileOffset,
                    [In, MarshalAs(UnmanagedType.U8)] ulong fragmentSize,
                    [Out] void** fragmentContext
            );

                /// <summary>
                /// Releases a fragment from a file.
                /// </summary>
                /// <param name="fragmentContext">The client defined context of a font fragment returned from ReadFileFragment.</param>
                [PreserveSig]
                unsafe void ReleaseFileFragment(
                    [In] void* fragmentContext
                    );

                /// <summary>
                /// Obtains the total size of a file.
                /// </summary>
                /// <param name="fileSize">Receives the total size of the file.</param>
                /// <returns>
                /// Standard HRESULT error code.
                /// </returns>
                /// <remarks>
                /// Implementing GetFileSize() for asynchronously loaded font files may require
                /// downloading the complete file contents, therefore this method should only be used for operations that
                /// either require complete font file to be loaded (e.g., copying a font file) or need to make
                /// decisions based on the value of the file size (e.g., validation against a persisted file size).
                /// </remarks>
                [PreserveSig]
                unsafe int GetFileSize(
                    [Out/*, MarshalAs(UnmanagedType::U8)*/] ulong* fileSize
                    );

                /// <summary>
                /// Obtains the last modified time of the file. The last modified time is used by DirectWrite font selection algorithms
                /// to determine whether one font resource is more up to date than another one.
                /// </summary>
                /// <param name="lastWriteTime">Receives the last modifed time of the file in the format that represents
                /// the number of 100-nanosecond intervals since January 1, 1601 (UTC).</param>
                /// <returns>
                /// Standard HRESULT error code. For resources that don't have a concept of the last modified time, the implementation of
                /// GetLastWriteTime should return E_NOTIMPL.
                /// </returns>
                [PreserveSig]
                unsafe int GetLastWriteTime(
                    [Out/*, MarshalAs(UnmanagedType::U8)*/] ulong* lastWriteTime
                    );
            };


            /// <summary>
            /// Font file loader interface handles loading font file resources of a particular type from a key.
            /// The font file loader interface is recommended to be implemented by a singleton object.
            /// IMPORTANT: font file loader implementations must not register themselves with DirectWrite factory
            /// inside their constructors and must not unregister themselves in their destructors, because
            /// registration and unregistraton operations increment and decrement the object reference count respectively.
            /// Instead, registration and unregistration of font file loaders with DirectWrite factory should be performed
            /// outside of the font file loader implementation as a separate step.
            /// </summary>    
            [ComImport(), Guid("727cad4e-d6af-4c9e-8a08-d695b11caa49"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
            interface IDWriteFontFileLoaderMirror
            {
                /// <summary>
                /// Creates a font file stream object that encapsulates an open file resource.
                /// The resource is closed when the last reference to fontFileStream is released.
                /// </summary>
                /// <param name="fontFileReferenceKey">Font file reference key that uniquely identifies the font file resource
                /// within the scope of the font loader being used.</param>
                /// <param name="fontFileReferenceKeySize">Size of font file reference key in bytes.</param>
                /// <param name="fontFileStream">Pointer to the newly created font file stream.</param>
                /// <returns>
                /// Standard HRESULT error code.
                /// </returns>
                [PreserveSig]
                unsafe int CreateStreamFromKey(
                    [In] void* fontFileReferenceKey,
                    [In, MarshalAs(UnmanagedType.U4)] uint fontFileReferenceKeySize,
                    [Out/*, MarshalAs(UnmanagedType::Interface)*/] IntPtr* fontFileStream
                );
            }

            interface IClassification
            {
                void GetCharAttribute(
                    int unicodeScalar,
                    out bool isCombining,
                    out bool needsCaretInfo,
                    out bool isIndic,
                    out bool isDigit,
                    out bool isLatin,
                    out bool isStrong
                    );
            }

            [StructLayout(LayoutKind.Sequential)]
            internal unsafe struct DWriteScriptAnalysisNode
            {
                public DWRITE_SCRIPT_ANALYSIS Value;
                public fixed uint Range[2];
                public unsafe DWriteScriptAnalysisNode* Next;
            };

            [StructLayout(LayoutKind.Sequential)]
            internal unsafe struct DWriteNumberSubstitutionNode
            {
                public IntPtr Value;
                public fixed uint Range[2];
                public unsafe DWriteNumberSubstitutionNode* Next;
            };

            [Flags]
            enum CharAttribute
            {
                None = 0x00,
                IsCombining = 0x01,
                NeedsCaretInfo = 0x02,
                IsIndic = 0x04,
                IsLatin = 0x08,
                IsStrong = 0x10,
                IsExtended = 0x20
            };

            internal class ItemizerHelper
            {
                /// <summary>
                /// Helper function to set ItemFlags.HasExtendedCharacter.
                /// </summary>
                /// <remarks>
                /// The old WPF Itemizer used to set ItemFlags.HasExtendedCharacter flag to TRUE for 
                /// all characters that represent extended Unicode range (>U+FFFF), this includes Surrogates Area.
                ///     Surrogate Area: U+D800  - U+DFFF
                ///     Extended Area:  U+10000 - U+10FFFF
                /// </remarks>
                /// <param name="ch">Character to investigate.</param>
                /// <returns>Value indicating whether the character is in extended UTF-16 range or not.</returns>
                internal static bool IsExtendedCharacter(char ch)
                {
                    // NOTE: char is 16 bit, so values > U+FFFF cannot be expressed by char.
                    //       Hence check for ((ch & 0x1F0000) > 0) is not necessary.
                    return ((ch & 0xF800) == 0xD800);
                }
            };

            internal class TextItemizer
            {
                private unsafe DWriteScriptAnalysisNode* _pScriptAnalysisListHead;
                private unsafe DWriteNumberSubstitutionNode* _pNumberSubstitutionListHead;

                List<bool>           _isDigitList;
                List<uint[]>          _isDigitListRanges;

                public unsafe TextItemizer(DWriteScriptAnalysisNode* pScriptAnalysisListHead, DWriteNumberSubstitutionNode* pNumberSubstitutionListHead)
                {
                    _pScriptAnalysisListHead = pScriptAnalysisListHead;
                    _pNumberSubstitutionListHead = pNumberSubstitutionListHead;

                    _isDigitList = new List<bool>();
                    _isDigitListRanges = new List <uint[]>();
                }

                internal unsafe IList<Span> Itemize(CultureInfo numberCulture, CharAttribute[] charAttribute, uint textLength)
                {
                    DWriteScriptAnalysisNode* pScriptAnalysisListPrevious = _pScriptAnalysisListHead;
                    DWriteScriptAnalysisNode* pScriptAnalysisListCurrent = _pScriptAnalysisListHead;
                    int scriptAnalysisRangeIndex = 0;

                    DWriteNumberSubstitutionNode* pNumberSubstitutionListPrevious = _pNumberSubstitutionListHead;
                    DWriteNumberSubstitutionNode* pNumberSubstitutionListCurrent = _pNumberSubstitutionListHead;
                    uint numberSubstitutionRangeIndex = 0;

                    uint isDigitIndex = 0;
                    uint isDigitIndexOld = 0;
                    uint isDigitRangeIndex = 0;

                    uint rangeStart;
                    uint rangeEnd;

                    rangeEnd = GetNextSmallestPos(&pScriptAnalysisListCurrent, ref scriptAnalysisRangeIndex,
                                                  &pNumberSubstitutionListCurrent, ref numberSubstitutionRangeIndex,
                                                  ref isDigitIndex, ref isDigitRangeIndex);

                    List <Span> spanVector = new List <Span>();
                    while (
                        rangeEnd != textLength
                        && (pScriptAnalysisListCurrent != null
                        || pNumberSubstitutionListCurrent != null
                        || isDigitIndex < (uint)_isDigitList.Count)
                        )
                    {
                        rangeStart = rangeEnd;
                        while (rangeEnd == rangeStart)
                        {
                            pScriptAnalysisListPrevious = pScriptAnalysisListCurrent;
                            pNumberSubstitutionListPrevious = pNumberSubstitutionListCurrent;
                            isDigitIndexOld = isDigitIndex;

                            rangeEnd = GetNextSmallestPos(&pScriptAnalysisListCurrent, ref scriptAnalysisRangeIndex,
                                                          &pNumberSubstitutionListCurrent, ref numberSubstitutionRangeIndex,
                                                          ref isDigitIndex, ref isDigitRangeIndex);
                        }

                        IntPtr pNumberSubstitution = IntPtr.Zero;
                        if (pNumberSubstitutionListPrevious != null
                         && rangeEnd > pNumberSubstitutionListPrevious->Range[0]
                         && rangeEnd <= pNumberSubstitutionListPrevious->Range[1])
                        {
                            pNumberSubstitution = pNumberSubstitutionListPrevious->Value;
                        }

                        // Assign HasCombiningMark
                        bool hasCombiningMark = false;
                        for (uint i = rangeStart; i < rangeEnd; ++i)
                        {
                            if ((charAttribute[i] & CharAttribute.IsCombining) != 0)
                            {
                                hasCombiningMark = true;
                                break;
                            }
                        }

                        // Assign NeedsCaretInfo
                        // When NeedsCaretInfo is false (and the run does not contain any combining marks)
                        // this makes caret navigation happen on the character level 
                        // and not the cluster level. When we have an itemized run based on DWrite logic
                        // that contains more than one WPF 3.5 scripts (based on unicode 3.x) we might run
                        // into a rare scenario where one script allows, for example, ligatures and the other not.
                        // In that case we default to false and let the combining marks check (which checks for
                        // simple and complex combining marks) decide whether character or cluster navigation
                        // will happen for the current run.
                        bool needsCaretInfo = true;
                        for (uint i = rangeStart; i < rangeEnd; ++i)
                        {
                            // Does NOT need caret info
                            if (((charAttribute[i] & CharAttribute.IsStrong) != 0) && ((charAttribute[i] & CharAttribute.NeedsCaretInfo) == 0))
                            {
                                needsCaretInfo = false;
                                break;
                            }
                        }

                        int strongCharCount = 0;
                        int latinCount = 0;
                        int indicCount = 0;
                        bool hasExtended = false;
                        for (uint i = rangeStart; i < rangeEnd; ++i)
                        {
                            if ((charAttribute[i] & CharAttribute.IsExtended) != 0)
                            {
                                hasExtended = true;
                            }


                            // If the current character class is Strong.
                            if ((charAttribute[i] & CharAttribute.IsStrong) != 0)
                            {
                                strongCharCount++;

                                if ((charAttribute[i] & CharAttribute.IsLatin) != 0)
                                {
                                    latinCount++;
                                }
                                else if ((charAttribute[i] & CharAttribute.IsIndic) != 0)
                                {
                                    indicCount++;
                                }
                            }
                        }

                        // Assign isIndic
                        // For the isIndic check we mark the run as Indic if it contains atleast
                        // one strong Indic character based on the old WPF 3.5 script ids.
                        // The isIndic flag is eventually used by LS when checking for the max cluster
                        // size that can form for the current run so that it can break the line properly.
                        // So our approach is conservative. 1 strong Indic character will make us 
                        // communicate to LS the max cluster size possible for correctness.
                        bool isIndic = (indicCount > 0);

                        // Assign isLatin
                        // We mark a run to be Latin iff all the strong characters in it
                        // are Latin based on the old WPF 3.5 script ids.
                        // This is a conservative approach for correct line breaking behavior.
                        // Refer to the comment about isIndic above.
                        bool isLatin = (strongCharCount > 0) && (latinCount == strongCharCount);

                        ItemProps itemProps = ItemProps.Create(
                                &(pScriptAnalysisListPrevious->Value),
                                pNumberSubstitution,
                                _isDigitList[(int)isDigitIndexOld] ? numberCulture : null,
                                hasCombiningMark,
                                needsCaretInfo,
                                hasExtended,
                                isIndic,
                                isLatin
                                );

                        spanVector.Add(new Span(itemProps, (int)(rangeEnd - rangeStart)));
                    }

                    return spanVector;
                }

                private unsafe uint GetNextSmallestPos(DWriteScriptAnalysisNode** ppScriptAnalysisCurrent, ref int scriptAnalysisRangeIndex, DWriteNumberSubstitutionNode** ppNumberSubstitutionCurrent, ref uint numberSubstitutionRangeIndex, ref uint isDigitIndex, ref uint isDigitRangeIndex)
                {
                    uint scriptAnalysisPos = (*ppScriptAnalysisCurrent != null) ? (*ppScriptAnalysisCurrent)->Range[scriptAnalysisRangeIndex] : uint.MaxValue;
                    uint numberSubPos = (*ppNumberSubstitutionCurrent != null) ? (*ppNumberSubstitutionCurrent)->Range[numberSubstitutionRangeIndex] : uint.MaxValue;
                    uint isDigitPos = (isDigitIndex < (uint)_isDigitListRanges.Count) ? _isDigitListRanges[(int)isDigitIndex][isDigitRangeIndex] : uint.MaxValue;

                    uint smallestPos = Math.Min(scriptAnalysisPos, numberSubPos);
                    smallestPos = Math.Min(smallestPos, isDigitPos);
                    if (smallestPos == scriptAnalysisPos)
                    {
                        if ((scriptAnalysisRangeIndex + 1) / 2 == 1)
                        {
                            (*ppScriptAnalysisCurrent) = (*ppScriptAnalysisCurrent)->Next;
                        }
                        scriptAnalysisRangeIndex = (scriptAnalysisRangeIndex + 1) % 2;
                    }
                    else if (smallestPos == numberSubPos)
                    {
                        if ((numberSubstitutionRangeIndex + 1) / 2 == 1)
                        {
                            (*ppNumberSubstitutionCurrent) = (*ppNumberSubstitutionCurrent)->Next;
                        }
                        numberSubstitutionRangeIndex = (numberSubstitutionRangeIndex + 1) % 2;
                    }
                    else
                    {
                        isDigitIndex += (isDigitRangeIndex + 1) / 2;
                        isDigitRangeIndex = (isDigitRangeIndex + 1) % 2;
                    }
                    return smallestPos;
                }

                internal void SetIsDigit(uint textPosition, uint textLength, bool isDigit)
                {
                    _isDigitList.Add(isDigit);
                    uint[] range = new uint[2];
                    range[0] = textPosition;
                    range[1] = textPosition + textLength;
                    _isDigitListRanges.Add(range);
                }
            }

            internal struct DWRITE_FONT_FEATURE
            {

            }

            [StructLayout(LayoutKind.Sequential)]
            internal struct DWRITE_TYPOGRAPHIC_FEATURES
            {
                internal unsafe DWRITE_FONT_FEATURE* features;
                internal uint featureCount;
            }

            internal class TextAnalyzer
            {
                IntPtr _textAnalyzer;

                internal TextAnalyzer(IntPtr textAnalyzer)
                {
                    _textAnalyzer = textAnalyzer;
                }

                private const int DWRITE_SCRIPT_SHAPES_DEFAULT = 0;

                public static char CharHyphen
                {
                    get { return '\x002d'; }
                }

                internal static unsafe IList<Span> Itemize(char* text,
                                                           uint length,
                                                           CultureInfo culture,
                                                           Factory factory,
                                                           bool isRightToLeftParagraph,
                                                           CultureInfo numberCulture,
                                                           bool ignoreUserOverride,
                                                           uint numberSubstitutionMethod,
                                                           IClassification classificationUtility,
                                                           CreateTextAnalysisSink createTextAnalysisSink,
                                                           GetScriptAnalysisList getScriptAnalysisList,
                                                           GetNumberSubstitutionList getNumberSubstitutionList,
                                                           CreateTextAnalysisSource createTextAnalysisSource)
                {
                    // If a text has zero length then we do not need to itemize.
                    if (length > 0)
                    {
                        IntPtr pTextAnalyzer = IntPtr.Zero;
                        void* pTextAnalysisSink = null;
                        void* pTextAnalysisSource = null;

                        // We obtain an AddRef factory so as not to worry about having to call GC::KeepAlive(factory)
                        // which puts unnecessary maintenance cost on this code.
                        IntPtr pDWriteFactory = factory.DWriteFactoryAddRef;
                        int hr = HRESULT.S_OK;
                        try
                        {
                            NativeMethods.FactoryCreateTextAnalyzer(pDWriteFactory, out pTextAnalyzer);
                            string languageTag = (culture != null) ? culture.IetfLanguageTag : "";
                            string numberLanguageString = (numberCulture != null) ? numberCulture.IetfLanguageTag : "";

                            fixed (char* cultureString = languageTag)
                            {
                                fixed (char* numberCultureString = numberLanguageString)
                                {
                                    // NOTE: the text parameter is NOT copied inside TextAnalysisSource to improve perf.
                                    // This is ok as long as we use the TextAnalysisSource in the same scope as we hold ref to text.
                                    // If we are ever to change this pattern then this should be revisited in TextAnalysisSource in
                                    // PresentationNative.
                                    hr = createTextAnalysisSource(text,
                                                                length,
                                                                cultureString,
                                                                (void*)(pDWriteFactory),
                                                                isRightToLeftParagraph,
                                                                numberCultureString,
                                                                ignoreUserOverride,
                                                                numberSubstitutionMethod,
                                                                &pTextAnalysisSource);
                                }
                            }

                            pTextAnalysisSink = createTextAnalysisSink();

                            // Analyze the script ranges.
                            hr = NativeMethods.TextAnalyzerAnalyzeScript(pTextAnalyzer, pTextAnalysisSource,
                                                             0,
                                                             length,
                                                             pTextAnalysisSink);

                            // Analyze the number substitution ranges.
                            hr = NativeMethods.TextAnalyzerAnalyzeNumberSubstitution(pTextAnalyzer,
                                                                        pTextAnalysisSource,
                                                                        0,
                                                                        length,
                                                                        pTextAnalysisSink);

                            DWriteScriptAnalysisNode* dwriteScriptAnalysisNode = (DWriteScriptAnalysisNode*)getScriptAnalysisList((void*)pTextAnalysisSink);
                            DWriteNumberSubstitutionNode* dwriteNumberSubstitutionNode = (DWriteNumberSubstitutionNode*)getNumberSubstitutionList((void*)pTextAnalysisSink);

                            TextItemizer textItemizer = new TextItemizer(dwriteScriptAnalysisNode, dwriteNumberSubstitutionNode);

                            return AnalyzeExtendedAndItemize(textItemizer, text, length, numberCulture, classificationUtility);
                        }
                        finally
                        {
                            // Review and re-add this once we can verify
                            System.Diagnostics.Debug.WriteLine("Release resources.");
                            /*
                            ReleaseItemizationNativeResources(pDWriteFactory,
                                                              pTextAnalyzer,
                                                              pTextAnalysisSource,
                                                              pTextAnalysisSink);
                            */
                        }
                    }
                    else
                    {
                        return null;
                    }
                }

                private static unsafe IList<Span> AnalyzeExtendedAndItemize(TextItemizer textItemizer, char* text, uint length, CultureInfo numberCulture, IClassification classification)
                {
                    Invariant.Assert(length >= 0);

                    CharAttribute[] charAttribute = new CharAttribute[length];

                    // Analyze the extended character ranges.
                    AnalyzeExtendedCharactersAndDigits(text, length, textItemizer, charAttribute, numberCulture, classification);
                    return textItemizer.Itemize(numberCulture, charAttribute, length);
                }

                
                private static unsafe void AnalyzeExtendedCharactersAndDigits(char* text, uint length, TextItemizer textItemizer, CharAttribute[] charAttribute, CultureInfo numberCulture, IClassification classificationUtility)
                {
                    // Text will never be of zero length. This is enforced by Itemize().
                    bool isCombining = false;
                    bool needsCaretInfo = false;
                    bool isIndic = false;
                    bool isDigit = false;
                    bool isLatin = false;
                    bool isStrong = false;
                    bool isExtended = false;

                    classificationUtility.GetCharAttribute(
                        text[0],
                        out isCombining,
                        out needsCaretInfo,
                        out isIndic,
                        out isDigit,
                        out isLatin,
                        out isStrong
                        );

                    isExtended = ItemizerHelper.IsExtendedCharacter(text[0]);

                    uint isDigitRangeStart = 0;
                    uint isDigitRangeEnd = 0;
                    bool previousIsDigitValue = (numberCulture == null) ? false : isDigit;
                    bool currentIsDigitValue;

                    // charAttribute is assumed to have the same length as text. This is enforced by Itemize().
                    charAttribute[0] = (CharAttribute)
                                        (((isCombining) ? CharAttribute.IsCombining : CharAttribute.None)
                                       | ((needsCaretInfo) ? CharAttribute.NeedsCaretInfo : CharAttribute.None)
                                       | ((isLatin) ? CharAttribute.IsLatin : CharAttribute.None)
                                       | ((isIndic) ? CharAttribute.IsIndic : CharAttribute.None)
                                       | ((isStrong) ? CharAttribute.IsStrong : CharAttribute.None)
                                       | ((isExtended) ? CharAttribute.IsExtended : CharAttribute.None));

                    uint i;
                    for (i = 1; i < length; ++i)
                    {
                        classificationUtility.GetCharAttribute(
                        text[i],
                        out isCombining,
                        out needsCaretInfo,
                        out isIndic,
                        out isDigit,
                        out isLatin,
                        out isStrong
                        );

                        isExtended = ItemizerHelper.IsExtendedCharacter(text[i]);


                        charAttribute[i] = (CharAttribute)
                                            (((isCombining) ? CharAttribute.IsCombining : CharAttribute.None)
                                           | ((needsCaretInfo) ? CharAttribute.NeedsCaretInfo : CharAttribute.None)
                                           | ((isLatin) ? CharAttribute.IsLatin : CharAttribute.None)
                                           | ((isIndic) ? CharAttribute.IsIndic : CharAttribute.None)
                                           | ((isStrong) ? CharAttribute.IsStrong : CharAttribute.None)
                                           | ((isExtended) ? CharAttribute.IsExtended : CharAttribute.None));


                        currentIsDigitValue = (numberCulture == null) ? false : isDigit;
                        if (previousIsDigitValue != currentIsDigitValue)
                        {

                            isDigitRangeEnd = i;
                            textItemizer.SetIsDigit(isDigitRangeStart, isDigitRangeEnd - isDigitRangeStart, previousIsDigitValue);

                            isDigitRangeStart = i;
                            previousIsDigitValue = currentIsDigitValue;
                        }
                    }

                    isDigitRangeEnd = i;
                    textItemizer.SetIsDigit(isDigitRangeStart, isDigitRangeEnd - isDigitRangeStart, previousIsDigitValue);
                }

                internal unsafe void GetGlyphsAndTheirPlacements(char* textString, uint textLength, Font font, ushort blankGlyphIndex, bool isSideways, bool isRightToLeft, CultureInfo cultureInfo, DWriteFontFeature[][] features, uint[] featureRangeLengths, double fontEmSize, double scalingFactor, float pixelsPerDip, TextFormattingMode textFormattingMode, ItemProps itemProps, out ushort[] clusterMap, out ushort[] glyphIndices, out int[] glyphAdvances, out GlyphOffset[] glyphOffsets)
                {
                    glyphIndices = null;
                    uint maxGlyphCount = 3 * textLength;
                    clusterMap = new ushort[textLength];
                    fixed (ushort* pclusterMapPinned = &clusterMap[0])
                    {
                        ushort[] textProps = new ushort[textLength];

                        fixed(ushort* textPropsPinned = &textProps[0])
                        {
                            uint[] glyphProps = null;

                            uint actualGlyphCount = maxGlyphCount + 1;

                            // Loop and everytime increase the size of the GlyphIndices buffer.
                            while (actualGlyphCount > maxGlyphCount)
                            {
                                maxGlyphCount = actualGlyphCount;
                                glyphProps = new uint[maxGlyphCount];
                                glyphIndices = new ushort[maxGlyphCount];

                                fixed (uint* glyphPropsPinned = &glyphProps[0])
                                {
                                    fixed (ushort* glyphIndicesPinned = &glyphIndices[0])
                                    {
                                        GetGlyphs(
                                        textString,
                                        textLength,
                                        font,
                                        blankGlyphIndex,
                                        isSideways,
                                        isRightToLeft,
                                        cultureInfo,
                                        features,
                                        featureRangeLengths,
                                        maxGlyphCount,
                                        textFormattingMode,
                                        itemProps,
                                        pclusterMapPinned,
                                        textPropsPinned,
                                        glyphIndicesPinned,
                                        glyphPropsPinned,
                                        null,
                                        out actualGlyphCount
                                        );
                                    }
                                }
                            }

                            Array.Resize(ref glyphIndices, (int)actualGlyphCount);
                            glyphAdvances = new int[actualGlyphCount];
                            fixed (int* glyphAdvancesPinned = &glyphAdvances[0])
                            {
                                fixed (uint* glyphPropsPinned = &glyphProps[0])
                                {
                                    fixed (ushort* glyphIndicesPinned = &glyphIndices[0])
                                    {
                                        glyphOffsets = new GlyphOffset[actualGlyphCount];
                                        GetGlyphPlacements(
                                            textString,
                                            pclusterMapPinned,
                                            textPropsPinned,
                                            textLength,
                                            glyphIndicesPinned,
                                            glyphPropsPinned,
                                            actualGlyphCount,
                                            font,
                                            fontEmSize,
                                            scalingFactor,
                                            isSideways,
                                            isRightToLeft,
                                            cultureInfo,
                                            features,
                                            featureRangeLengths,
                                            textFormattingMode,
                                            itemProps,
                                            pixelsPerDip,
                                            glyphAdvancesPinned,
                                            out glyphOffsets
                                            );
                                    }
                                }
                            }
                        }
                    }
                }

                internal unsafe void GetGlyphs(char* textString, uint textLength, Font font, ushort blankGlyphIndex, bool isSideways, bool isRightToLeft, CultureInfo cultureInfo, DWriteFontFeature[][] features, uint[] featureRangeLengths, uint maxGlyphCount, TextFormattingMode textFormattingMode, ItemProps itemProps, ushort* clusterMap, ushort* textProps, ushort* glyphIndices, uint* glyphProps, int* pfCanGlyphAlone, out uint actualGlyphCount)
                {
                    // Shaping should not have taken place if ScriptAnalysis is NULL!
                    Invariant.Assert(itemProps.ScriptAnalysis != null);

                    actualGlyphCount = 0;

                    // These are control characters and WPF will not display control characters.
                    if (itemProps.ScriptAnalysis->shapes != DWRITE_SCRIPT_SHAPES_DEFAULT)
                    {
                        FontFace fontFace = font.GetFontFace();
                        try
                        {
                            GetBlankGlyphsForControlCharacters(
                                textString,
                                textLength,
                                fontFace,
                                blankGlyphIndex,
                                maxGlyphCount,
                                clusterMap,
                                glyphIndices,
                                pfCanGlyphAlone,
                                out actualGlyphCount
                                );
                        }
                        finally
                        {
                            fontFace.Release();
                        }
                    }
                    else
                    {
                        String localeName = cultureInfo.IetfLanguageTag;

                        uint featureRanges = 0;
                        GCHandle[] dwriteFontFeaturesGCHandles = null;
                        DWRITE_TYPOGRAPHIC_FEATURES*[] dwriteTypographicFeatures = null;
                        DWRITE_TYPOGRAPHIC_FEATURES[] dwriteTypographicFeaturesStorage = null;

                        if (features != null)
                        {
                            featureRanges = (uint)featureRangeLengths.Length;
                            dwriteFontFeaturesGCHandles = new GCHandle[featureRanges];
                            dwriteTypographicFeatures = new DWRITE_TYPOGRAPHIC_FEATURES*[featureRanges];
                            dwriteTypographicFeaturesStorage = new DWRITE_TYPOGRAPHIC_FEATURES[featureRanges];
                        }

                        fixed (DWRITE_TYPOGRAPHIC_FEATURES** pFeaturesArray = &dwriteTypographicFeatures[0])
                        {
                            fixed (DWRITE_TYPOGRAPHIC_FEATURES* pFeature = &dwriteTypographicFeaturesStorage[0])
                            {
                                FontFace fontFace = font.GetFontFace();
                                try
                                {
                                    if (features != null)
                                    {
                                        for (uint i = 0; i < featureRanges; ++i)
                                        {
                                            dwriteFontFeaturesGCHandles[i] = GCHandle.Alloc(features[i], GCHandleType.Pinned);
                                            dwriteTypographicFeatures[i] = pFeature + i;
                                            dwriteTypographicFeatures[i]->features = (DWRITE_FONT_FEATURE*)(dwriteFontFeaturesGCHandles[i].AddrOfPinnedObject().ToPointer());
                                            dwriteTypographicFeatures[i]->featureCount = (uint)features[i].Length;
                                        }
                                    }

                                    uint glyphCount = 0;

                                    int hr = NativeMethods.TextAnalyzerGetGlyphs(_textAnalyzer,
                                        textString,
                                        /*checked*/((uint)textLength),
                                        fontFace.DWriteFontFaceNoAddRef,
                                        isSideways,
                                        isRightToLeft,
                                        itemProps.ScriptAnalysis,
                                        localeName,
                                        itemProps.NumberSubstitutionNoAddRef,
                                        pFeaturesArray,
                                        featureRangeLengths,
                                        featureRanges,
                                        /*checked*/((uint)maxGlyphCount),
                                        clusterMap,
                                        textProps, //The size of DWRITE_SHAPING_TEXT_PROPERTIES is 16 bits which is the same size that LS passes to WPF 
                                                   //Thus we can safely cast textProps to DWRITE_SHAPING_TEXT_PROPERTIES*
                                        glyphIndices,
                                        glyphProps, //The size of DWRITE_SHAPING_GLYPH_PROPERTIES is 16 bits. LS passes a pointer to UINT32 so this cast is safe since 
                                                    //We will not write into memory outside the boundary. But this is cast will result in an unused space. We are taking this
                                                    //Approach for now to avoid modifying LS code.
                                        out glyphCount
                                        );

                                    if (MS.Internal.Interop.HRESULT.E_INVALIDARG.Code == hr)
                                    {
                                        // If pLocaleName is unsupported (e.g. "prs-af"), DWrite returns E_INVALIDARG.
                                        // Try again with the default mapping.
                                        hr = NativeMethods.TextAnalyzerGetGlyphs(_textAnalyzer,
                                            textString,
                                            textLength,
                                            fontFace.DWriteFontFaceNoAddRef,
                                            isSideways,
                                            isRightToLeft,
                                            itemProps.ScriptAnalysis,
                                            null,
                                            itemProps.NumberSubstitutionNoAddRef,
                                            pFeaturesArray,
                                            featureRangeLengths,
                                            featureRanges,
                                            /*checked*/((uint)maxGlyphCount),
                                            clusterMap,
                                            textProps, //The size of DWRITE_SHAPING_TEXT_PROPERTIES is 16 bits which is the same size that LS passes to WPF 
                                                       //Thus we can safely cast textProps to DWRITE_SHAPING_TEXT_PROPERTIES*
                                            glyphIndices,
                                            glyphProps, //The size of DWRITE_SHAPING_GLYPH_PROPERTIES is 16 bits. LS passes a pointer to UINT32 so this cast is safe since 
                                                        //We will not write into memory outside the boundary. But this is cast will result in an unused space. We are taking this
                                                        //Approach for now to avoid modifying LS code.
                                            out glyphCount
                                            );
                                    }

                                    if (features != null)
                                    {
                                        for (uint i = 0; i < featureRanges; ++i)
                                        {
                                            dwriteFontFeaturesGCHandles[i].Free();
                                        }
                                    }

                                    const int HR_ERROR_INSUFFICIENT_BUFFER = unchecked((int)0x8007007A);
                                    if (hr == HR_ERROR_INSUFFICIENT_BUFFER)
                                    {
                                        // Actual glyph count is not returned by DWrite unless the call tp GetGlyphs succeeds.
                                        // It must be re-estimated in case the first estimate was not adequate.
                                        // The following calculation is a refactoring of DWrite's logic ( 3*stringLength/2 + 16) after 3 retries.
                                        // We'd rather go directly to the maximum buffer size we are willing to allocate than pay the cost of continuously retrying.
                                        actualGlyphCount = 27 * maxGlyphCount / 8 + 76;
                                    }
                                    else
                                    {
                                        if (pfCanGlyphAlone != null)
                                        {
                                            for (uint i = 0; i < textLength; ++i)
                                            {
                                                bool isShapedAlone = (textProps[i] & 1) != 0;
                                                pfCanGlyphAlone[i] = isShapedAlone ? 1 : 0;
                                            }
                                        }

                                        actualGlyphCount = glyphCount;
                                    }
                                }
                                finally
                                {
                                    fontFace.Release();
                                }
                            }
                        }
                    }
                }

                private unsafe void GetBlankGlyphsForControlCharacters(char* pTextString, uint textLength, FontFace fontFace, ushort blankGlyphIndex, uint maxGlyphCount, ushort* clusterMap, ushort* glyphIndices, int* pfCanGlyphAlone, out uint actualGlyphCount)
                {
                    actualGlyphCount = textLength;
                    // There is not enough buffer allocated. Signal to the caller the correct buffer size.
                    if (maxGlyphCount < textLength)
                    {
                        return;
                    }

                    if (textLength > ushort.MaxValue)
                    {
                        throw new InvalidOperationException();
                    }

                    ushort textLengthShort = (ushort)textLength;

                    uint softHyphen = (uint)CharHyphen;
                    ushort hyphenGlyphIndex = 0;
                    for (ushort i = 0; i < textLengthShort; ++i)
                    {
                        // LS will sometimes replace soft hyphens (which are invisible) with hyphens (which are visible).
                        // So if we are in this code then LS actually did this replacement and we need to display the hyphen (x002D)
                        if (pTextString[i] == CharHyphen)
                        {
                            if (hyphenGlyphIndex == 0)
                            {
                                NativeMethods.FontFaceGetGlyphIndices(fontFace.DWriteFontFaceNoAddRef,
                                                            &softHyphen,
                                                            1,
                                                            &hyphenGlyphIndex
                                                            );
                            }
                            glyphIndices[i] = hyphenGlyphIndex;
                        }
                        else
                        {
                            glyphIndices[i] = blankGlyphIndex;
                        }
                        clusterMap[i] = i;
                        pfCanGlyphAlone[i] = 1;
                    }
                }

                internal unsafe void GetGlyphPlacements(char* textString, ushort* clusterMap, ushort* textProps, uint textLength, ushort* glyphIndices, uint* glyphProps, uint glyphCount, Font font, double fontEmSize, double scalingFactor, bool isSideways, bool isRightToLeft, CultureInfo cultureInfo, DWriteFontFeature[][] features, uint[] featureRangeLengths, TextFormattingMode textFormattingMode, ItemProps itemProps, float pixelsPerDip, int* glyphAdvances, out GlyphOffset[] glyphOffsets)
                {
                    glyphOffsets = null;

                    // Shaping should not have taken place if ScriptAnalysis is NULL!
                    Invariant.Assert(itemProps.ScriptAnalysis != null);

                    // These are control characters and WPF will not display control characters.
                    if (itemProps.ScriptAnalysis->shapes != DWRITE_SCRIPT_SHAPES_DEFAULT)
                    {
                        GetGlyphPlacementsForControlCharacters(
                            textString,
                            textLength,
                            font,
                            textFormattingMode,
                            fontEmSize,
                            scalingFactor,
                            isSideways,
                            pixelsPerDip,
                            glyphCount,
                            glyphIndices,
                            glyphAdvances,
                            glyphOffsets
                            );
                    }
                    else
                    {
                        float[] dwriteGlyphAdvances = new float[glyphCount];
                        DWRITE_GLYPH_OFFSET[] dwriteGlyphOffsets = new DWRITE_GLYPH_OFFSET[glyphCount];

                        List<GCHandle> dwriteFontFeaturesGCHandles = null;
                        uint featureRanges = 0;
                        DWRITE_TYPOGRAPHIC_FEATURES*[] dwriteTypographicFeatures = null;

                        fixed (uint* pFeatureRangeLengthsPinned = &featureRangeLengths[0])
                        {
                            uint* pFeatureRangeLengths = null;
                            if (features != null)
                            {
                                featureRanges = (uint)featureRangeLengths.Length;
                                dwriteTypographicFeatures = new DWRITE_TYPOGRAPHIC_FEATURES*[featureRanges];
                                pFeatureRangeLengths = pFeatureRangeLengthsPinned;
                                dwriteFontFeaturesGCHandles = new List<GCHandle>();
                            }

                            fixed (DWRITE_TYPOGRAPHIC_FEATURES** dwriteTypographicFeaturesPinned = dwriteTypographicFeatures)
                            {

                                FontFace fontFace = font.GetFontFace();
                                try
                                {
                                    DWRITE_MATRIX transform = Factory.GetIdentityTransform();

                                    if (features != null)
                                    {
                                        for (uint i = 0; i < featureRanges; ++i)
                                        {
                                            GCHandle arrayAlloc = GCHandle.Alloc(features[i], GCHandleType.Pinned);
                                            dwriteFontFeaturesGCHandles.Add(arrayAlloc);

                                            DWRITE_TYPOGRAPHIC_FEATURES typographicFeatures = new DWRITE_TYPOGRAPHIC_FEATURES();
                                            GCHandle typographicFeaturesAlloc = GCHandle.Alloc(typographicFeatures, GCHandleType.Pinned);
                                            dwriteFontFeaturesGCHandles.Add(typographicFeaturesAlloc);

                                            dwriteTypographicFeatures[i] = (DWRITE_TYPOGRAPHIC_FEATURES*)(typographicFeaturesAlloc.AddrOfPinnedObject().ToPointer());
                                            dwriteTypographicFeatures[i]->features = (DWRITE_FONT_FEATURE*)(arrayAlloc.AddrOfPinnedObject().ToPointer());
                                            dwriteTypographicFeatures[i]->featureCount = (uint)features[i].Length;
                                        }
                                    }

                                    float fontEmSizeFloat = (float)fontEmSize;
                                    int hr = HRESULT.E_FAIL;

                                    if (textFormattingMode == TextFormattingMode.Ideal)
                                    {
                                        hr = NativeMethods.TextAnalyzerGetGlyphPlacements(
                                            _textAnalyzer,
                                            textString,
                                            clusterMap,
                                            textProps,
                                            textLength,
                                            glyphIndices,
                                            glyphProps,
                                            glyphCount,
                                            fontFace.DWriteFontFaceNoAddRef,
                                            fontEmSizeFloat,
                                            isSideways,
                                            isRightToLeft,
                                            itemProps.ScriptAnalysis,
                                            cultureInfo.IetfLanguageTag,
                                            dwriteTypographicFeaturesPinned,
                                            pFeatureRangeLengths,
                                            featureRanges,
                                            dwriteGlyphAdvances,
                                            dwriteGlyphOffsets
                                            );

                                        if (MS.Internal.Interop.HRESULT.E_INVALIDARG.Code == hr)
                                        {
                                            // If pLocaleName is unsupported (e.g. "prs-af"), DWrite returns E_INVALIDARG.
                                            // Try again with the default mapping.
                                            hr = NativeMethods.TextAnalyzerGetGlyphPlacements(
                                                _textAnalyzer,
                                                textString,
                                                clusterMap,
                                                textProps,
                                                textLength,
                                                glyphIndices,
                                                glyphProps,
                                                glyphCount,
                                                fontFace.DWriteFontFaceNoAddRef,
                                                fontEmSizeFloat,
                                                isSideways,
                                                isRightToLeft,
                                                itemProps.ScriptAnalysis,
                                                null /* default locale mapping */,
                                                dwriteTypographicFeaturesPinned,
                                                pFeatureRangeLengths,
                                                featureRanges,
                                                dwriteGlyphAdvances,
                                                dwriteGlyphOffsets
                                                );
                                        }

                                    }
                                    else
                                    {
                                        Invariant.Assert(textFormattingMode == TextFormattingMode.Display);

                                        hr = NativeMethods.TextAnalyzerGetGdiCompatibleGlyphPlacements(
                                            _textAnalyzer,
                                            textString,
                                            clusterMap,
                                            textProps,
                                            textLength,
                                            glyphIndices,
                                            glyphProps,
                                            glyphCount,
                                            fontFace.DWriteFontFaceNoAddRef,
                                            fontEmSizeFloat,
                                            pixelsPerDip,
                                            &transform,
                                            false,  // useGdiNatural
                                            isSideways,
                                            isRightToLeft,
                                            itemProps.ScriptAnalysis,
                                            cultureInfo.IetfLanguageTag,
                                            dwriteTypographicFeaturesPinned,
                                            pFeatureRangeLengths,
                                            featureRanges,
                                            dwriteGlyphAdvances,
                                            dwriteGlyphOffsets
                                            );

                                        if (MS.Internal.Interop.HRESULT.E_INVALIDARG.Code == hr)
                                        {
                                            // If pLocaleName is unsupported (e.g. "prs-af"), DWrite returns E_INVALIDARG.
                                            // Try again with the default mapping.
                                            hr = NativeMethods.TextAnalyzerGetGdiCompatibleGlyphPlacements(
                                                _textAnalyzer,
                                                textString,
                                                clusterMap,
                                                textProps,
                                                textLength,
                                                glyphIndices,
                                                glyphProps,
                                                glyphCount,
                                                fontFace.DWriteFontFaceNoAddRef,
                                                fontEmSizeFloat,
                                                pixelsPerDip,
                                                &transform,
                                                false,  // useGdiNatural
                                                isSideways,
                                                isRightToLeft,
                                                itemProps.ScriptAnalysis,
                                                null /* default locale mapping */,
                                                dwriteTypographicFeaturesPinned,
                                                pFeatureRangeLengths,
                                                featureRanges,
                                                dwriteGlyphAdvances,
                                                dwriteGlyphOffsets
                                                );
                                        }
                                    }

                                    if (features != null)
                                    {
                                        for (int i = 0; i < dwriteFontFeaturesGCHandles.Count; ++i)
                                        {
                                            dwriteFontFeaturesGCHandles[i].Free();
                                        }
                                    }

                                    glyphOffsets = new GlyphOffset[glyphCount];
                                    if (textFormattingMode == TextFormattingMode.Ideal)
                                    {
                                        for (uint i = 0; i < glyphCount; ++i)
                                        {
                                            glyphAdvances[i] = (int)Math.Round(dwriteGlyphAdvances[i] * fontEmSize * scalingFactor / fontEmSizeFloat);
                                            glyphOffsets[i].du = (int)(dwriteGlyphOffsets[i].advanceOffset * scalingFactor);
                                            glyphOffsets[i].dv = (int)(dwriteGlyphOffsets[i].ascenderOffset * scalingFactor);
                                        }
                                    }
                                    else
                                    {
                                        for (uint i = 0; i < glyphCount; ++i)
                                        {
                                            glyphAdvances[i] = (int)Math.Round(dwriteGlyphAdvances[i] * scalingFactor);
                                            glyphOffsets[i].du = (int)(dwriteGlyphOffsets[i].advanceOffset * scalingFactor);
                                            glyphOffsets[i].dv = (int)(dwriteGlyphOffsets[i].ascenderOffset * scalingFactor);
                                        }
                                    }
                                }
                                finally
                                {
                                    fontFace.Release();
                                }
                            }
                        }
                    }
                }

                private unsafe void GetGlyphPlacementsForControlCharacters(char* textString, uint textLength, Font font, TextFormattingMode textFormattingMode, double fontEmSize, double scalingFactor, bool isSideways, float pixelsPerDip, uint glyphCount, ushort* glyphIndices, int* glyphAdvances, GlyphOffset[] glyphOffsets)
                {
                    if (glyphCount != textLength)
                    {
                        throw new Exception();
                    }

                    glyphOffsets = new GlyphOffset[textLength];
                    FontFace fontFace = font.GetFontFace();

                    int hyphenAdvanceWidth = -1;
                    for (uint i = 0; i < textLength; ++i)
                    {
                        // LS will sometimes replace soft hyphens (which are invisible) with hyphens (which are visible).
                        // So if we are in this code then LS actually did this replacement and we need to display the hyphen (x002D)
                        if (textString[i] == CharHyphen)
                        {
                            if (hyphenAdvanceWidth == -1)
                            {
                                GlyphMetrics glyphMetrics = new GlyphMetrics();

                                if (textFormattingMode == TextFormattingMode.Ideal)
                                {
                                    NativeMethods.FontFaceGetDesignGlyphMetrics(fontFace.DWriteFontFaceNoAddRef,
                                                                                    glyphIndices + i,
                                                                                    1,
                                                                                    &glyphMetrics
                                                                                    );
                                }
                                else
                                {
                                    NativeMethods.FontFaceGetGdiCompatibleGlyphMetrics(fontFace.DWriteFontFaceNoAddRef,
                                                                                    (float)fontEmSize,
                                                                                    pixelsPerDip, //FLOAT pixelsPerDip,
                                                                                    null, // optional transform
                                                                                    textFormattingMode != TextFormattingMode.Display, //BOOL useGdiNatural,
                                                                                    glyphIndices + i, //__in_ecount(glyphCount) UINT16 const* glyphIndices,
                                                                                    1, //UINT32 glyphCount,
                                                                                    &glyphMetrics, //__out_ecount(glyphCount) DWRITE_GLYPH_METRICS* glyphMetrics
                                                                                    isSideways
                                                                                    );
                                }
                                double approximatedHyphenAW = Math.Round(glyphMetrics.AdvanceWidth * fontEmSize / font.Metrics.DesignUnitsPerEm * pixelsPerDip) / pixelsPerDip;
                                hyphenAdvanceWidth = (int)Math.Round(approximatedHyphenAW * scalingFactor);
                            }

                            glyphAdvances[i] = hyphenAdvanceWidth;
                        }
                        else
                        {
                            glyphAdvances[i] = 0;
                        }
                        glyphOffsets[i].du = 0;
                        glyphOffsets[i].dv = 0;
                    }
                }
            }
        }
    }
}


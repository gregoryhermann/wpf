// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

//
// This file specifies various assembly level attributes.
//

using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Security;
using System.Security.Permissions;
using System.Windows.Markup;

[assembly:Dependency("mscorlib,", LoadHint.Always)]
[assembly:Dependency("System,", LoadHint.Always)]
[assembly:Dependency("System.Xml,", LoadHint.Sometimes)]

[assembly:XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml", "System.Windows.Markup")]

namespace System.Xaml.Permissions
{
    public sealed class XamlLoadPermission : CodeAccessPermission, IUnrestrictedPermission
    {
        public XamlLoadPermission(PermissionState state) { }
        public XamlLoadPermission(XamlAccessLevel allowedAccess) { }
        public XamlLoadPermission(IEnumerable<XamlAccessLevel> allowedAccess) { }
        [ComVisible(false)]
        public override bool Equals(object obj) { return ReferenceEquals(this, obj); }
        [ComVisible(false)]
        public override int GetHashCode() { return base.GetHashCode(); }
        public IList<XamlAccessLevel> AllowedAccess { get; private set; } = new ReadOnlyCollection<XamlAccessLevel>(Array.Empty<XamlAccessLevel>());
        public override IPermission Copy() { return new XamlLoadPermission(PermissionState.Unrestricted); }
        public override void FromXml(SecurityElement elem) { }
        public bool Includes(XamlAccessLevel requestedAccess) { return true; }
        public override IPermission Intersect(IPermission target) { return new XamlLoadPermission(PermissionState.Unrestricted); }
        public override bool IsSubsetOf(IPermission target) { return true; }
        public override SecurityElement ToXml() { return default(SecurityElement); }
        public override IPermission Union(IPermission other) { return new XamlLoadPermission(PermissionState.Unrestricted); }
        public bool IsUnrestricted() { return true; }
    }
}

namespace System.Xaml.Permissions
{
    public class XamlAccessLevel
    {
        private XamlAccessLevel(string assemblyName, string typeName)
        {
            AssemblyNameString = assemblyName;
            PrivateAccessToTypeName = typeName;
        }

        public static XamlAccessLevel AssemblyAccessTo(Assembly assembly)
        {
            return new XamlAccessLevel(assembly.FullName, null);
        }

        public static XamlAccessLevel AssemblyAccessTo(AssemblyName assemblyName)
        {
            return new XamlAccessLevel(assemblyName.FullName, null);
        }

        public static XamlAccessLevel PrivateAccessTo(Type type)
        {
            return new XamlAccessLevel(type.Assembly.FullName, type.FullName);
        }

        public static XamlAccessLevel PrivateAccessTo(string assemblyQualifiedTypeName)
        {
            int nameBoundary = assemblyQualifiedTypeName.IndexOf(',');
            string typeName = assemblyQualifiedTypeName.Substring(0, nameBoundary).Trim().ToString();
            string assemblyFullName = assemblyQualifiedTypeName.Substring(nameBoundary + 1).Trim().ToString();
            AssemblyName assemblyName = new AssemblyName(assemblyFullName);
            return new XamlAccessLevel(assemblyName.FullName, typeName);
        }

        public AssemblyName AssemblyAccessToAssemblyName
        {
            get { return new AssemblyName(AssemblyNameString); }
        }

        public string PrivateAccessToTypeName { get; private set; }

        internal string AssemblyNameString { get; private set; }
    }
}

namespace System.Windows.Markup
{
    /// <summary>
    /// Attribute to associate a ValueSerializer class with a value type or to override
    /// which value serializer to use for a property. A value serializer can be associated
    /// with an attached property by placing the attribute on the static accessor for the
    /// attached property.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Interface | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Property | AttributeTargets.Method, AllowMultiple = false, Inherited = true)]    
    public sealed class ValueSerializerAttribute : Attribute
    {
        private Type? _valueSerializerType;
        private readonly string? _valueSerializerTypeName;

        /// <summary>
        /// Constructor for the ValueSerializerAttribute
        /// </summary>
        /// <param name="valueSerializerType">Type of the value serializer being associated with a type or property</param>
        public ValueSerializerAttribute(Type valueSerializerType)
        {
            _valueSerializerType = valueSerializerType;
        }

        /// <summary>
        /// Constructor for the ValueSerializerAttribute
        /// </summary>
        /// <param name="valueSerializerTypeName">Fully qualified type name of the value serializer being associated with a type or property</param>
        public ValueSerializerAttribute(string valueSerializerTypeName)
        {
            _valueSerializerTypeName = valueSerializerTypeName;
        }

        /// <summary>
        /// The type of the value serializer to create for this type or property.
        /// </summary>
        public Type ValueSerializerType
        {
            get
            {
                if (_valueSerializerType == null && _valueSerializerTypeName != null)
                {
                    _valueSerializerType = Type.GetType(_valueSerializerTypeName);
                }

                return _valueSerializerType!;
            }
        }

        /// <summary>
        /// The assembly qualified name of the value serializer type for this type or property.
        /// </summary>
        public string ValueSerializerTypeName
        {
            get
            {
                if (_valueSerializerType != null)
                {
                    return _valueSerializerType.AssemblyQualifiedName!;
                }
                else
                {
                    return _valueSerializerTypeName!;
                }
            }
        }
    }
}

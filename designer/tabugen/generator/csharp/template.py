
CS_CONV_TEMPLATE = """
public class Conv
{
    public static bool ParseBool(string s)
    {
        if (string.IsNullOrEmpty(s) || s.Equals("0", StringComparison.OrdinalIgnoreCase))
        {
            return false;
        }
        if (s.Equals("1", StringComparison.OrdinalIgnoreCase))
        {
            return true;
        }
        return bool.Parse(s);
    }

    public static byte ParseByte(string s)
    {
        return string.IsNullOrEmpty(s) ? (byte)0 : byte.Parse(s);
    }

    public static sbyte ParseSbyte(string s)
    {
        return string.IsNullOrEmpty(s) ? (sbyte)0 : sbyte.Parse(s);
    }

    public static short ParseShort(string s)
    {
        return string.IsNullOrEmpty(s) ? (short)0 : short.Parse(s);
    }

    public static ushort ParseUShort(string s)
    {
        return string.IsNullOrEmpty(s) ? (ushort)0 : ushort.Parse(s);
    }

    public static int ParseInt(string s)
    {
        return string.IsNullOrEmpty(s) ? 0 : int.Parse(s);
    }

    public static uint ParseUInt(string s)
    {
        return string.IsNullOrEmpty(s) ? 0 : uint.Parse(s);
    }

    public static long ParseLong(string s)
    {
        return string.IsNullOrEmpty(s) ? 0 : long.Parse(s);
    }

    public static ulong ParseULong(string s)
    {
        return string.IsNullOrEmpty(s) ? 0 : ulong.Parse(s);
    }

    public static float ParseFloat(string s)
    {
        return string.IsNullOrEmpty(s) ? 0 : float.Parse(s);
    }

    public static double ParseDouble(string s)
    {
        return string.IsNullOrEmpty(s) ? 0 : double.Parse(s);
    }

    public static T[] ParseArray<T>(string s)
    {
        var list = new List<T>();
        var parts = s.Split("|", StringSplitOptions.RemoveEmptyEntries);
        for (int i = 0; i < parts.Length; i++)
        {
            if (parts[i].Length > 0)
            {
                T v = (T)Convert.ChangeType(parts[i], typeof(T));
                list.Add(v);
            }
        }
        return list.ToArray();
    }

    public static Dictionary<K,V> ParseMap<K,V>(string s) where K: notnull
    {
        var dict = new Dictionary<K, V>();
        var parts = s.Split("|", StringSplitOptions.RemoveEmptyEntries);
        for (int i = 0; i < parts.Length; i++)
        {
            var pair = parts[i].Split("=", StringSplitOptions.RemoveEmptyEntries);
            if (pair.Length == 2)
            {
                K key = (K)Convert.ChangeType(pair[0], typeof(K));
                V val = (V)Convert.ChangeType(pair[1], typeof(V));
                dict[key] = val;
            }
        }
        return dict;
    }

    public static string GetFieldValue(Dictionary<string, string> record, string key)
    {
        string value;
        record.TryGetValue(key, out value);
        return value;
    }
}
"""

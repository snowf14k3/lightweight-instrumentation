# Simple-Instrumentation

---
## features
- classTransform
- redefineClass
- retransformClass

---
## Usage
```java

@Test
public void test(){
    SClassTransformer.list.add(new CustomTransfomer());
}

```
---
```java
package com.utils;

import com.utils.Transformer;

public class SClassTransformer {
    /* add your custom Transfomer here*/
    public static ArrayList<Transformer> list = new ArrayList();

    public native static void redefineClass(Class<?> clazz,byte[] bytes);
    
    public native static void retransformClass(Class<?> clazz); 

    public static byte[] tansform(String name,byte[] orignalBytes){
       for(Transformer transfomer : SClassTransformer.list)
       {
           return transfomer.tansform(name,orignalBytes);
       } 
       return orignalBytes;
    }

    static{
        // ur lib load
        System.load("path/library.dll");
    }

}
```

---

```java
package com.utils;

public abstract class Transformer {
    public abstract byte[] tansform(String name,byte[] orignalBytes);
}
```

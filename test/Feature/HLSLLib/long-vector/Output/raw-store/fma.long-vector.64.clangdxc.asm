;
; Resource Bindings:
;
; Name                                 Type  Format         Dim      ID      HLSL Bind     Count
; ------------------------------ ---------- ------- ----------- ------- -------------- ---------
; In0                               texture    byte         r/o      T0             t0         1
; In1                               texture    byte         r/o      T1             t1         1
; In2                               texture    byte         r/o      T2             t2         1
; Out                                   UAV    byte         r/w      U0             u3         1
;
; ModuleID = 'D:\repos\offload-test-suite\test\Feature\HLSLLib\long-vector\Output\raw-store\source.hlsl'
source_filename = "D:\\repos\\offload-test-suite\\test\\Feature\\HLSLLib\\long-vector\\Output\\raw-store\\source.hlsl"
target datalayout = "e-m:e-ve-p:32:32-i1:32-i8:8-i16:16-i32:32-i64:64-f16:16-f32:32-f64:64-n8:16:32:64"
target triple = "dxilv1.0-unknown-shadermodel6.0-compute"

%ByteAddressBuffer = type { i32 }
%RWByteAddressBuffer = type { i32 }
%dx.types.Handle = type { ptr }
%dx.types.ResRet.i32 = type { i32, i32, i32, i32, i32 }
%dx.types.splitdouble = type { i32, i32 }

@In0 = external constant %ByteAddressBuffer
@In1 = external constant %ByteAddressBuffer
@In2 = external constant %ByteAddressBuffer
@Out = external constant %RWByteAddressBuffer

; Function Attrs: convergent noinline nounwind memory(readwrite, inaccessiblemem: none, target_mem: none)
define void @main() local_unnamed_addr #0 {
entry:
  %0 = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 0, i32 0, i32 0, i1 false)
  %1 = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 0, i32 1, i32 1, i1 false)
  %2 = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 0, i32 2, i32 2, i1 false)
  %3 = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 0, i32 3, i1 false)
  %4 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %0, i32 0, i32 undef)
  %5 = extractvalue %dx.types.ResRet.i32 %4, 0
  %6 = extractvalue %dx.types.ResRet.i32 %4, 1
  %7 = extractvalue %dx.types.ResRet.i32 %4, 2
  %8 = extractvalue %dx.types.ResRet.i32 %4, 3
  %9 = call double @dx.op.makeDouble.f64(i32 101, i32 %5, i32 %6), !dx.precise !15
  %10 = call double @dx.op.makeDouble.f64(i32 101, i32 %7, i32 %8), !dx.precise !15
  %11 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %0, i32 0, i32 undef)
  %12 = extractvalue %dx.types.ResRet.i32 %11, 0
  %13 = extractvalue %dx.types.ResRet.i32 %11, 1
  %14 = extractvalue %dx.types.ResRet.i32 %11, 2
  %15 = extractvalue %dx.types.ResRet.i32 %11, 3
  %16 = call double @dx.op.makeDouble.f64(i32 101, i32 %12, i32 %13), !dx.precise !15
  %17 = call double @dx.op.makeDouble.f64(i32 101, i32 %14, i32 %15), !dx.precise !15
  %18 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %0, i32 32, i32 undef)
  %19 = extractvalue %dx.types.ResRet.i32 %18, 0
  %20 = extractvalue %dx.types.ResRet.i32 %18, 1
  %21 = extractvalue %dx.types.ResRet.i32 %18, 2
  %22 = extractvalue %dx.types.ResRet.i32 %18, 3
  %23 = call double @dx.op.makeDouble.f64(i32 101, i32 %19, i32 %20), !dx.precise !15
  %24 = call double @dx.op.makeDouble.f64(i32 101, i32 %21, i32 %22), !dx.precise !15
  %25 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %0, i32 32, i32 undef)
  %26 = extractvalue %dx.types.ResRet.i32 %25, 0
  %27 = extractvalue %dx.types.ResRet.i32 %25, 1
  %28 = extractvalue %dx.types.ResRet.i32 %25, 2
  %29 = extractvalue %dx.types.ResRet.i32 %25, 3
  %30 = call double @dx.op.makeDouble.f64(i32 101, i32 %26, i32 %27), !dx.precise !15
  %31 = call double @dx.op.makeDouble.f64(i32 101, i32 %28, i32 %29), !dx.precise !15
  %32 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %1, i32 0, i32 undef)
  %33 = extractvalue %dx.types.ResRet.i32 %32, 0
  %34 = extractvalue %dx.types.ResRet.i32 %32, 1
  %35 = extractvalue %dx.types.ResRet.i32 %32, 2
  %36 = extractvalue %dx.types.ResRet.i32 %32, 3
  %37 = call double @dx.op.makeDouble.f64(i32 101, i32 %33, i32 %34), !dx.precise !15
  %38 = call double @dx.op.makeDouble.f64(i32 101, i32 %35, i32 %36), !dx.precise !15
  %39 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %1, i32 0, i32 undef)
  %40 = extractvalue %dx.types.ResRet.i32 %39, 0
  %41 = extractvalue %dx.types.ResRet.i32 %39, 1
  %42 = extractvalue %dx.types.ResRet.i32 %39, 2
  %43 = extractvalue %dx.types.ResRet.i32 %39, 3
  %44 = call double @dx.op.makeDouble.f64(i32 101, i32 %40, i32 %41), !dx.precise !15
  %45 = call double @dx.op.makeDouble.f64(i32 101, i32 %42, i32 %43), !dx.precise !15
  %46 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %1, i32 32, i32 undef)
  %47 = extractvalue %dx.types.ResRet.i32 %46, 0
  %48 = extractvalue %dx.types.ResRet.i32 %46, 1
  %49 = extractvalue %dx.types.ResRet.i32 %46, 2
  %50 = extractvalue %dx.types.ResRet.i32 %46, 3
  %51 = call double @dx.op.makeDouble.f64(i32 101, i32 %47, i32 %48), !dx.precise !15
  %52 = call double @dx.op.makeDouble.f64(i32 101, i32 %49, i32 %50), !dx.precise !15
  %53 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %1, i32 32, i32 undef)
  %54 = extractvalue %dx.types.ResRet.i32 %53, 0
  %55 = extractvalue %dx.types.ResRet.i32 %53, 1
  %56 = extractvalue %dx.types.ResRet.i32 %53, 2
  %57 = extractvalue %dx.types.ResRet.i32 %53, 3
  %58 = call double @dx.op.makeDouble.f64(i32 101, i32 %54, i32 %55), !dx.precise !15
  %59 = call double @dx.op.makeDouble.f64(i32 101, i32 %56, i32 %57), !dx.precise !15
  %60 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %2, i32 0, i32 undef)
  %61 = extractvalue %dx.types.ResRet.i32 %60, 0
  %62 = extractvalue %dx.types.ResRet.i32 %60, 1
  %63 = extractvalue %dx.types.ResRet.i32 %60, 2
  %64 = extractvalue %dx.types.ResRet.i32 %60, 3
  %65 = call double @dx.op.makeDouble.f64(i32 101, i32 %61, i32 %62), !dx.precise !15
  %66 = call double @dx.op.makeDouble.f64(i32 101, i32 %63, i32 %64), !dx.precise !15
  %67 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %2, i32 0, i32 undef)
  %68 = extractvalue %dx.types.ResRet.i32 %67, 0
  %69 = extractvalue %dx.types.ResRet.i32 %67, 1
  %70 = extractvalue %dx.types.ResRet.i32 %67, 2
  %71 = extractvalue %dx.types.ResRet.i32 %67, 3
  %72 = call double @dx.op.makeDouble.f64(i32 101, i32 %68, i32 %69), !dx.precise !15
  %73 = call double @dx.op.makeDouble.f64(i32 101, i32 %70, i32 %71), !dx.precise !15
  %74 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %2, i32 32, i32 undef)
  %75 = extractvalue %dx.types.ResRet.i32 %74, 0
  %76 = extractvalue %dx.types.ResRet.i32 %74, 1
  %77 = extractvalue %dx.types.ResRet.i32 %74, 2
  %78 = extractvalue %dx.types.ResRet.i32 %74, 3
  %79 = call double @dx.op.makeDouble.f64(i32 101, i32 %75, i32 %76), !dx.precise !15
  %80 = call double @dx.op.makeDouble.f64(i32 101, i32 %77, i32 %78), !dx.precise !15
  %81 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %2, i32 32, i32 undef)
  %82 = extractvalue %dx.types.ResRet.i32 %81, 0
  %83 = extractvalue %dx.types.ResRet.i32 %81, 1
  %84 = extractvalue %dx.types.ResRet.i32 %81, 2
  %85 = extractvalue %dx.types.ResRet.i32 %81, 3
  %86 = call double @dx.op.makeDouble.f64(i32 101, i32 %82, i32 %83), !dx.precise !15
  %87 = call double @dx.op.makeDouble.f64(i32 101, i32 %84, i32 %85), !dx.precise !15
  %.i045336 = call double @dx.op.tertiary.f64(i32 47, double %9, double %37, double %65)
  %.i146335 = call double @dx.op.tertiary.f64(i32 47, double %10, double %38, double %66)
  %.i247334 = call double @dx.op.tertiary.f64(i32 47, double %16, double %44, double %72)
  %.i348333 = call double @dx.op.tertiary.f64(i32 47, double %17, double %45, double %73)
  %.i4332 = call double @dx.op.tertiary.f64(i32 47, double %23, double %51, double %79)
  %.i5331 = call double @dx.op.tertiary.f64(i32 47, double %24, double %52, double %80)
  %.i6330 = call double @dx.op.tertiary.f64(i32 47, double %30, double %58, double %86)
  %.i7329 = call double @dx.op.tertiary.f64(i32 47, double %31, double %59, double %87)
  %.i049344 = call %dx.types.splitdouble @dx.op.splitDouble.f64(i32 102, double %.i045336)
  %.i150343 = call %dx.types.splitdouble @dx.op.splitDouble.f64(i32 102, double %.i146335)
  %.i251342 = call %dx.types.splitdouble @dx.op.splitDouble.f64(i32 102, double %.i247334)
  %.i352341 = call %dx.types.splitdouble @dx.op.splitDouble.f64(i32 102, double %.i348333)
  %.elem0 = extractvalue %dx.types.splitdouble %.i049344, 0
  %.elem053 = extractvalue %dx.types.splitdouble %.i150343, 0
  %.elem054 = extractvalue %dx.types.splitdouble %.i251342, 0
  %.elem055 = extractvalue %dx.types.splitdouble %.i352341, 0
  %.elem1 = extractvalue %dx.types.splitdouble %.i049344, 1
  %.elem156 = extractvalue %dx.types.splitdouble %.i150343, 1
  %.elem157 = extractvalue %dx.types.splitdouble %.i251342, 1
  %.elem158 = extractvalue %dx.types.splitdouble %.i352341, 1
  call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle %3, i32 0, i32 undef, i32 %.elem0, i32 %.elem1, i32 %.elem053, i32 %.elem156, i8 15)
  call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle %3, i32 0, i32 undef, i32 %.elem054, i32 %.elem157, i32 %.elem055, i32 %.elem158, i8 15)
  %.i059340 = call %dx.types.splitdouble @dx.op.splitDouble.f64(i32 102, double %.i4332)
  %.i160339 = call %dx.types.splitdouble @dx.op.splitDouble.f64(i32 102, double %.i5331)
  %.i261338 = call %dx.types.splitdouble @dx.op.splitDouble.f64(i32 102, double %.i6330)
  %.i362337 = call %dx.types.splitdouble @dx.op.splitDouble.f64(i32 102, double %.i7329)
  %.elem063 = extractvalue %dx.types.splitdouble %.i059340, 0
  %.elem064 = extractvalue %dx.types.splitdouble %.i160339, 0
  %.elem065 = extractvalue %dx.types.splitdouble %.i261338, 0
  %.elem066 = extractvalue %dx.types.splitdouble %.i362337, 0
  %.elem167 = extractvalue %dx.types.splitdouble %.i059340, 1
  %.elem168 = extractvalue %dx.types.splitdouble %.i160339, 1
  %.elem169 = extractvalue %dx.types.splitdouble %.i261338, 1
  %.elem170 = extractvalue %dx.types.splitdouble %.i362337, 1
  call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle %3, i32 32, i32 undef, i32 %.elem063, i32 %.elem167, i32 %.elem064, i32 %.elem168, i8 15)
  call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle %3, i32 32, i32 undef, i32 %.elem065, i32 %.elem169, i32 %.elem066, i32 %.elem170, i8 15)
  ret void
}

; Function Attrs: nounwind memory(read)
declare %dx.types.Handle @dx.op.createHandle(i32, i8, i32, i32, i1) #1

; Function Attrs: nounwind memory(read)
declare %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32, %dx.types.Handle, i32, i32) #1

; Function Attrs: nounwind memory(none)
declare double @dx.op.makeDouble.f64(i32, i32, i32) #2

; Function Attrs: nounwind
declare void @dx.op.bufferStore.i32(i32, %dx.types.Handle, i32, i32, i32, i32, i32, i32, i8) #3

; Function Attrs: nounwind memory(none)
declare double @dx.op.tertiary.f64(i32, double, double, double) #2

; Function Attrs: nounwind memory(none)
declare %dx.types.splitdouble @dx.op.splitDouble.f64(i32, double) #2

attributes #0 = { convergent noinline nounwind memory(readwrite, inaccessiblemem: none, target_mem: none) }
attributes #1 = { nounwind memory(read) }
attributes #2 = { nounwind memory(none) }
attributes #3 = { nounwind }

!dx.valver = !{!0}
!llvm.ident = !{!1}
!dx.shaderModel = !{!2}
!dx.version = !{!3}
!dx.resources = !{!4}
!dx.entryPoints = !{!11}
!llvm.module.flags = !{!14}

!0 = !{i32 1, i32 8}
!1 = !{!"clang version 24.0.0git (https://github.com/llvm/llvm-project.git aa261b730fe1dd212160de5ba64ed2a71a20b72c)"}
!2 = !{!"cs", i32 6, i32 0}
!3 = !{i32 1, i32 0}
!4 = !{!5, !9, null, null}
!5 = !{!6, !7, !8}
!6 = !{i32 0, ptr @In0, !"In0", i32 0, i32 0, i32 1, i32 11, i32 0, null}
!7 = !{i32 1, ptr @In1, !"In1", i32 0, i32 1, i32 1, i32 11, i32 0, null}
!8 = !{i32 2, ptr @In2, !"In2", i32 0, i32 2, i32 1, i32 11, i32 0, null}
!9 = !{!10}
!10 = !{i32 0, ptr @Out, !"Out", i32 0, i32 3, i32 1, i32 11, i1 false, i1 false, i1 false, null}
!11 = !{ptr @main, !"main", null, !4, !12}
!12 = !{i32 0, i64 84, i32 4, !13}
!13 = !{i32 1, i32 1, i32 1}
!14 = !{i32 2, !"frame-pointer", i32 2}
!15 = !{i32 1}

; RUN: llvm-as %s -f -o %t1.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee --output-dir=%t.klee-out -disable-opt --exit-on-error %t1.bc > %t2
;
; ModuleID = '<stdin>'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@0 = private unnamed_addr constant [22 x i8] c"!check_inverses(A, B)\00", align 1
@1 = private unnamed_addr constant [92 x i8] c"/home/dan/dev/klee/fp-bench/benchmarks/c/imperial/synthetic/matrix_inverse/matrix_inverse.c\00", align 1
@2 = private unnamed_addr constant [11 x i8] c"int main()\00", align 1

; Function Attrs: nounwind readnone uwtable
define zeroext i1 @very_close(float, float) #0 {
  %3 = fsub float %0, %1
  %4 = tail call float @fabsf(float %3) #5
  %5 = fcmp olt float %4, 0x3E80000000000000
  ret i1 %5
}

; Function Attrs: nounwind uwtable
define void @matrix_mul([2 x float]* nocapture readonly, [2 x float]* nocapture readonly, [2 x float]* nocapture) #1 {
  %4 = getelementptr inbounds [2 x float]* %0, i64 0, i64 0
  %5 = load float* %4, align 4, !tbaa !3
  %6 = getelementptr inbounds [2 x float]* %1, i64 0, i64 0
  %7 = load float* %6, align 4, !tbaa !3
  %8 = fmul float %5, %7
  %9 = fadd float %8, 0.000000e+00
  %10 = getelementptr inbounds [2 x float]* %0, i64 0, i64 1
  %11 = load float* %10, align 4, !tbaa !3
  %12 = getelementptr inbounds [2 x float]* %1, i64 1, i64 0
  %13 = load float* %12, align 4, !tbaa !3
  %14 = fmul float %11, %13
  %15 = fadd float %9, %14
  %16 = getelementptr inbounds [2 x float]* %2, i64 0, i64 0
  store float %15, float* %16, align 4, !tbaa !3
  %17 = load float* %4, align 4, !tbaa !3
  %18 = getelementptr inbounds [2 x float]* %1, i64 0, i64 1
  %19 = load float* %18, align 4, !tbaa !3
  %20 = fmul float %17, %19
  %21 = fadd float %20, 0.000000e+00
  %22 = load float* %10, align 4, !tbaa !3
  %23 = getelementptr inbounds [2 x float]* %1, i64 1, i64 1
  %24 = load float* %23, align 4, !tbaa !3
  %25 = fmul float %22, %24
  %26 = fadd float %21, %25
  %27 = getelementptr inbounds [2 x float]* %2, i64 0, i64 1
  store float %26, float* %27, align 4, !tbaa !3
  %28 = getelementptr inbounds [2 x float]* %0, i64 1, i64 0
  %29 = load float* %28, align 4, !tbaa !3
  %30 = load float* %6, align 4, !tbaa !3
  %31 = fmul float %29, %30
  %32 = fadd float %31, 0.000000e+00
  %33 = getelementptr inbounds [2 x float]* %0, i64 1, i64 1
  %34 = load float* %33, align 4, !tbaa !3
  %35 = load float* %12, align 4, !tbaa !3
  %36 = fmul float %34, %35
  %37 = fadd float %32, %36
  %38 = getelementptr inbounds [2 x float]* %2, i64 1, i64 0
  store float %37, float* %38, align 4, !tbaa !3
  %39 = load float* %28, align 4, !tbaa !3
  %40 = load float* %18, align 4, !tbaa !3
  %41 = fmul float %39, %40
  %42 = fadd float %41, 0.000000e+00
  %43 = load float* %33, align 4, !tbaa !3
  %44 = load float* %23, align 4, !tbaa !3
  %45 = fmul float %43, %44
  %46 = fadd float %42, %45
  %47 = getelementptr inbounds [2 x float]* %2, i64 1, i64 1
  store float %46, float* %47, align 4, !tbaa !3
  ret void
}

; Function Attrs: nounwind readonly uwtable
define zeroext i1 @check_inverses([2 x float]* nocapture readonly, [2 x float]* nocapture readonly) #2 {
  %3 = alloca <4 x float>, align 16
  %4 = bitcast <4 x float>* %3 to [2 x [2 x float]]*
  %5 = getelementptr inbounds [2 x float]* %0, i64 0, i64 0
  %6 = load float* %5, align 4, !tbaa !3
  %7 = getelementptr inbounds [2 x float]* %1, i64 0, i64 0
  %8 = load float* %7, align 4, !tbaa !3
  %9 = getelementptr inbounds [2 x float]* %0, i64 0, i64 1
  %10 = load float* %9, align 4, !tbaa !3
  %11 = getelementptr inbounds [2 x float]* %1, i64 1, i64 0
  %12 = load float* %11, align 4, !tbaa !3
  %13 = getelementptr inbounds [2 x float]* %1, i64 0, i64 1
  %14 = load float* %13, align 4, !tbaa !3
  %15 = getelementptr inbounds [2 x float]* %1, i64 1, i64 1
  %16 = load float* %15, align 4, !tbaa !3
  %17 = getelementptr inbounds [2 x float]* %0, i64 1, i64 0
  %18 = load float* %17, align 4, !tbaa !3
  %19 = getelementptr inbounds [2 x float]* %0, i64 1, i64 1
  %20 = load float* %19, align 4, !tbaa !3
  %21 = insertelement <4 x float> undef, float %6, i32 0
  %22 = insertelement <4 x float> %21, float %6, i32 1
  %23 = insertelement <4 x float> %22, float %18, i32 2
  %24 = insertelement <4 x float> %23, float %18, i32 3
  %25 = insertelement <4 x float> undef, float %8, i32 0
  %26 = insertelement <4 x float> %25, float %14, i32 1
  %27 = insertelement <4 x float> %26, float %8, i32 2
  %28 = insertelement <4 x float> %27, float %14, i32 3
  %29 = fmul <4 x float> %24, %28
  %30 = fadd <4 x float> %29, zeroinitializer
  %31 = insertelement <4 x float> undef, float %10, i32 0
  %32 = insertelement <4 x float> %31, float %10, i32 1
  %33 = insertelement <4 x float> %32, float %20, i32 2
  %34 = insertelement <4 x float> %33, float %20, i32 3
  %35 = insertelement <4 x float> undef, float %12, i32 0
  %36 = insertelement <4 x float> %35, float %16, i32 1
  %37 = insertelement <4 x float> %36, float %12, i32 2
  %38 = insertelement <4 x float> %37, float %16, i32 3
  %39 = fmul <4 x float> %34, %38
  %40 = fadd <4 x float> %30, %39
  store <4 x float> %40, <4 x float>* %3, align 16, !tbaa !3
  br label %41

; <label>:41                                      ; preds = %61, %2
  %42 = phi i64 [ 0, %2 ], [ %62, %61 ]
  br label %43

; <label>:43                                      ; preds = %57, %41
  %44 = phi i64 [ 0, %41 ], [ %58, %57 ]
  %45 = trunc i64 %44 to i32
  %46 = trunc i64 %42 to i32
  %47 = icmp eq i32 %46, %45
  %48 = getelementptr inbounds [2 x [2 x float]]* %4, i64 0, i64 %42, i64 %44
  %49 = load float* %48, align 4, !tbaa !3
  br i1 %47, label %50, label %54

; <label>:50                                      ; preds = %43
  %51 = fadd float %49, -1.000000e+00
  %52 = tail call float @fabsf(float %51) #5
  %53 = fcmp olt float %52, 0x3E80000000000000
  br i1 %53, label %57, label %65

; <label>:54                                      ; preds = %43
  %55 = tail call float @fabsf(float %49) #5
  %56 = fcmp olt float %55, 0x3E80000000000000
  br i1 %56, label %57, label %65

; <label>:57                                      ; preds = %54, %50
  %58 = add nuw nsw i64 %44, 1
  %59 = trunc i64 %58 to i32
  %60 = icmp slt i32 %59, 2
  br i1 %60, label %43, label %61

; <label>:61                                      ; preds = %57
  %62 = add nuw nsw i64 %42, 1
  %63 = trunc i64 %62 to i32
  %64 = icmp slt i32 %63, 2
  br i1 %64, label %41, label %65

; <label>:65                                      ; preds = %61, %54, %50
  %66 = phi i1 [ false, %50 ], [ false, %54 ], [ true, %61 ]
  ret i1 %66
}

; Function Attrs: nounwind uwtable
define i32 @main() #1 {
  %1 = alloca <4 x float>, align 16
  %2 = bitcast <4 x float>* %1 to i8*
  call void @llvm.lifetime.start(i64 16, i8* %2) #4
  %3 = bitcast <4 x float>* %1 to [2 x [2 x float]]*
  store <4 x float> <float fadd (float fadd (float fmul (float 1.000000e+00, float undef), float 0.000000e+00), float fmul (float 0.000000e+00, float undef)), float fadd (float fadd (float fmul (float 1.000000e+00, float undef), float 0.000000e+00), float fmul (float 0.000000e+00, float undef)), float fadd (float fadd (float fmul (float 0.000000e+00, float undef), float 0.000000e+00), float fmul (float 2.000000e+00, float undef)), float fadd (float fadd (float fmul (float 0.000000e+00, float undef), float 0.000000e+00), float fmul (float 2.000000e+00, float undef))>, <4 x float>* %1, align 16, !tbaa !3
  br label %4

; <label>:4                                       ; preds = %24, %0
  %5 = phi i64 [ 0, %0 ], [ %25, %24 ]
  %6 = trunc i64 %5 to i32
  br label %7

; <label>:7                                       ; preds = %20, %4
  %8 = phi i64 [ 0, %4 ], [ %21, %20 ]
  %9 = trunc i64 %8 to i32
  %10 = icmp eq i32 %6, %9
  %11 = getelementptr inbounds [2 x [2 x float]]* %3, i64 0, i64 %5, i64 %8
  %12 = load float* %11, align 4, !tbaa !3
  br i1 %10, label %13, label %17

; <label>:13                                      ; preds = %7
  %14 = fadd float %12, -1.000000e+00
  %15 = tail call float @fabsf(float %14) #5
  %16 = fcmp olt float %15, 0x3E80000000000000
  br i1 %16, label %20, label %29

; <label>:17                                      ; preds = %7
  %18 = tail call float @fabsf(float %12) #5
  %19 = fcmp olt float %18, 0x3E80000000000000
  br i1 %19, label %20, label %29

; <label>:20                                      ; preds = %17, %13
  %21 = add nuw nsw i64 %8, 1
  %22 = trunc i64 %21 to i32
  %23 = icmp slt i32 %22, 2
  br i1 %23, label %7, label %24

; <label>:24                                      ; preds = %20
  %25 = add nuw nsw i64 %5, 1
  %26 = trunc i64 %25 to i32
  %27 = icmp slt i32 %26, 2
  br i1 %27, label %4, label %28

; <label>:28                                      ; preds = %24
  call void @llvm.lifetime.end(i64 16, i8* %2) #4
  tail call void @__assert_fail(i8* getelementptr inbounds ([22 x i8]* @0, i64 0, i64 0), i8* getelementptr inbounds ([92 x i8]* @1, i64 0, i64 0), i32 120, i8* getelementptr inbounds ([11 x i8]* @2, i64 0, i64 0)) #6
  unreachable

; <label>:29                                      ; preds = %17, %13
  call void @llvm.lifetime.end(i64 16, i8* %2) #4
  ret i32 0
}

; Function Attrs: noreturn nounwind
declare void @__assert_fail(i8*, i8*, i32, i8*) #3

declare float @fabsf(float)

; Function Attrs: nounwind
declare void @llvm.lifetime.start(i64, i8* nocapture) #4

; Function Attrs: nounwind
declare void @llvm.lifetime.end(i64, i8* nocapture) #4

attributes #0 = { nounwind readnone uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind readonly uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { noreturn nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind }
attributes #5 = { nounwind readnone "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #6 = { noreturn nounwind }

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!2}

!0 = metadata !{i32 2, metadata !"Dwarf Version", i32 4}
!1 = metadata !{i32 1, metadata !"Debug Info Version", i32 1}
!2 = metadata !{metadata !"clang version 3.4.2 (tags/RELEASE_34/dot2-final)"}
!3 = metadata !{metadata !4, metadata !4, i64 0}
!4 = metadata !{metadata !"float", metadata !5, i64 0}
!5 = metadata !{metadata !"omnipotent char", metadata !6, i64 0}
!6 = metadata !{metadata !"Simple C/C++ TBAA"}

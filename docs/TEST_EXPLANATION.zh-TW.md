# CI 測試說明文件

**專案：** XRI Demo — X 光檢測 CI 概念驗證
**文件目的：** 說明 CI 流程中執行的全部自動化測試：每個測試套件的用途、各測試案例驗證的內容,以及這些測試如何在無螢幕(headless)環境下執行。

---

## 總覽

CI 每次 push / pull request 都會在 **Linux(GCC)** 與 **Windows(MSVC)** 兩個作業系統上執行 **6 個測試套件**,全部透過 CTest 驅動:

| # | 測試套件 | 類型 | 測試對象 | 執行模式 |
| --- | --- | --- | --- | --- |
| 1 | `tst_syntheticxray` | 單元測試 | 合成 X 光影像產生器 | 無 GUI(`QTEST_GUILESS_MAIN`) |
| 2 | `tst_scansimulator` | 單元測試 | 掃描模擬器(漸進式擷取) | 無 GUI |
| 3 | `tst_inspectionplan` | 單元測試 | 檢測框(ROI)資料模型 | 無 GUI |
| 4 | `tst_algorithms` | 單元測試 | 四種檢測演算法與檢測引擎 | 無 GUI |
| 5 | `tst_exporter` | 單元測試 | 報告匯出(PNG/JSON/CSV/HTML) | 離屏(offscreen) |
| 6 | `tst_smoke_mainwindow` | UI 冒煙測試 | 完整主視窗操作流程 | 離屏(offscreen) |

兩種執行模式的差別:

- **無 GUI(guiless)**:只建立 `QCoreApplication`,完全不碰視窗系統,任何環境都能執行。
- **離屏(offscreen)**:透過 Qt 的 `offscreen` 平台外掛執行完整的視窗與事件系統,但不需要實體螢幕或顯示伺服器。`QT_QPA_PLATFORM=offscreen` 是設定在 **CTest 屬性**上,而不是 CI 的 YAML 裡,因此開發者本機執行 `ctest` 與 CI 上的行為完全一致。

---

## 1. `tst_syntheticxray` — 合成 X 光影像產生器

**為什麼重要:** 整個測試體系的基礎是「同樣的參數必須產生同樣的影像」。影像可重現,演算法的輸出才能被精確斷言。

| 測試案例 | 驗證內容 |
| --- | --- |
| `sizeAndFormat` | 產生的影像尺寸與要求一致(例如 320×200),且格式為 8 位元灰階(`Format_Grayscale8`) |
| `deterministicForSameSeed` | 相同的亂數種子(seed)連續產生兩次,影像**逐位元組完全相同** — 這是確定性(determinism)的核心保證 |
| `differentSeedsDiffer` | 不同的種子必須產生不同的影像,確認種子真的有作用 |
| `meanIntensityPlausible` | 整張影像的平均灰階值落在合理區間(100~230),確認「亮背景 + 較暗物件」的輻射影像特性成立 |
| `defectDarkensImage` | 開啟「注入瑕疵」後,影像中最暗的像素必須比未注入時**明顯更暗**(差距 > 15 灰階),證明瑕疵真的被畫進影像,且暗到能被演算法偵測 |

> 實作上特別避開了 `std::normal_distribution`(MSVC 與 libc++ 的輸出序列不同),改用自行實作的 Box–Muller 轉換,確保跨平台的確定性。

---

## 2. `tst_scansimulator` — 掃描模擬器

**為什麼重要:** 模擬器模仿線掃描偵測器逐帶(band-by-band)成像的行為,介面與真實硬體驅動相同。這展示了「硬體相依程式碼如何在 CI 中測試」的標準模式。

| 測試案例 | 驗證內容 |
| --- | --- |
| `emitsMonotonicProgressAndFinishes` | 掃描進度訊號(`progressChanged`)單調遞增、最終到達 100%,且 `finished` 訊號恰好發出一次 |
| `finishedImageMatchesDirectGeneration` | 掃描完成後送出的影像,與直接呼叫影像產生器(相同參數)的結果**完全一致** — 漸進式顯示不會改變最終資料 |
| `cancelStopsTheScan` | 呼叫 `cancel()` 後計時器停止、發出 `cancelled` 訊號,且**不會**再發出 `finished` |

> 測試將計時器間隔設為 0(`setTickInterval(0)`),整個掃描在事件迴圈內瞬間完成 — **CI 中沒有任何真實時間等待**,這是讓 UI 相關測試保持快速且穩定的關鍵設計。

---

## 3. `tst_inspectionplan` — 檢測框資料模型

**為什麼重要:** 檢測框(ROI)是設定頁、演算法頁、匯出功能共用的核心資料。模型錯誤會以「畫面偶發錯亂或當機」的形式出現在產品中,極難從客訴重現 — 在單元測試層攔截是最便宜的位置。

| 測試案例 | 驗證內容 |
| --- | --- |
| `addRemoveUpdate` | 新增/修改/刪除檢測框的基本操作正確,自動命名(ROI 1、ROI 2…)正確,且每個操作都發出對應的訊號 |
| `updateUnknownIdFails` | 更新不存在的 ID 時回傳失敗,不會造成未定義行為 |
| `normalizesRectOnAdd` | 使用者「反向拖曳」(從右下拖到左上)畫出的矩形會被自動正規化為正常座標 |
| `jsonRoundTrip` | 整份檢測計畫序列化成 JSON 再讀回,內容完全一致;且自動命名計數器也被還原(讀回後新增的框不會重複命名) |
| `listModelFollowsPlan` | 清單模型(`RoiListModel`)正確跟隨計畫的增刪改;重新命名經由模型寫回計畫;空白名稱被拒絕。**並以 Qt 官方的 `QAbstractItemModelTester` 全程檢查模型契約** |

> **實際戰果:** `QAbstractItemModelTester` 在開發本專案時就攔截到一個真實缺陷 — 模型在資料已變動**之後**才宣告列的插入/刪除,違反 Qt 模型契約。此缺陷在第一次執行測試時被抓到、數分鐘內修復,並從此被永久回歸防護。

---

## 4. `tst_algorithms` — 檢測演算法與引擎

**為什麼重要:** 演算法是檢測軟體的核心價值。本套件採用「**手工建構、答案已知的影像**」策略:用程式畫出灰階值精確已知的測試圖,演算法的輸出就能被精確比對,而不是只驗證「大概對」。

| 測試案例 | 驗證內容 |
| --- | --- |
| `registryKnowsAllAlgorithms` | 演算法註冊表恰好有 4 個演算法,每個都能以 ID 查回、有顯示名稱與參數規格;查詢不存在的 ID 回傳空值 |
| `meanIntensityMeasuresExactly` | 在均勻灰階 100 的影像上,「平均亮度」演算法量測值**精確等於 100.0**;並測試判定門檻的邊界(門檻恰好等於量測值時應判 PASS,超過則 FAIL) |
| `densityRangeFindsOutliers` | 「密度範圍」演算法找得到影像中的暗點離群值;檢測框未涵蓋暗點時則不受影響 — 驗證 ROI 範圍限制正確 |
| `blobCountCountsDarkComponents` | 「暗斑計數」演算法:兩個分離的暗方塊算 2 個;面積低於門檻的小斑點不計;**兩個相鄰接觸的方塊以 4-連通合併為 1 個** — 驗證連通元件演算法本身的正確性 |
| `edgeStrengthDetectsStructure` | 「邊緣強度」演算法:有垂直階梯邊緣的影像量測值高、判 PASS;完全平坦的影像量測值精確為 0、判 FAIL |
| `engineRunsPlanAndAggregates` | 檢測引擎整合行為:已指派演算法的框被評估、未指派的被略過;任一框 FAIL 則整體判 FAIL;部分超出影像的框被裁切、完全超出的框不評估 |
| `engineWithNoAssignmentsFailsOverall` | 沒有任何框指派演算法時,整體判定為 FAIL(不可因「沒測任何東西」而判 PASS)— 這是檢測軟體的安全預設 |

---

## 5. `tst_exporter` — 報告匯出

**為什麼重要:** 匯出檔案是檢測結果的正式交付物。本套件驗證五種輸出格式都被產生且內容正確。

| 測試案例 | 驗證內容 |
| --- | --- |
| `exportsAllFormats` | 一次匯出後:(1) 五個檔案(原始 PNG、標註 PNG、JSON、CSV、HTML)全部存在;(2) JSON 可解析、ROI 數量與整體判定正確、掃描參數完整;(3) 標註影像與原始影像尺寸相同但**內容確實不同**(證明判定框真的被畫上去);(4) CSV 為表頭加上每個 ROI 一列 |
| `failsCleanlyWithoutImage` | 沒有掃描影像時,匯出乾淨地失敗:回傳錯誤訊息、不寫出任何檔案 — 不會產生半成品報告 |

> 測試寫入 `QTemporaryDir`(暫存目錄),結束後自動清除,CI 環境不殘留任何檔案。標註影像需要 QPainter 文字繪製,因此本套件以離屏模式執行。

---

## 6. `tst_smoke_mainwindow` — UI 冒煙測試(本概念驗證的核心證據)

**為什麼重要:** 這個測試直接回答「複雜 UI 能不能在 CI 中測試」。它驅動的是**真正的 `MainWindow`**(與使用者執行的完全相同的程式碼),在沒有螢幕的 CI 執行器上跑完整個操作流程。

單一測試案例 `fullWorkflow` 依序執行並斷言:

1. **側邊欄導覽** — 以合成滑鼠點擊依序點選四個頁面項目,斷言頁面堆疊(`QStackedWidget`)確實切換到對應頁面。
2. **完整掃描** — 將模擬器計時器設為 0,點擊「Start Scan」按鈕,斷言:掃描影像(640×480)進入共用工作階段(session),且應用程式自動跳轉到檢視頁。
3. **滑鼠拖曳畫出檢測框** — 切換到設定頁、點擊「Draw ROI」模式,接著在圖形檢視上合成「按下 → 移動 → 放開」的滑鼠拖曳事件,斷言:檢測計畫中出現一個尺寸正確的 ROI。這證明連 `QGraphicsView` 層級的滑鼠互動都能在無螢幕環境驗證。
4. **指派演算法並執行檢測** — 在演算法頁選取該 ROI、透過下拉選單指派「平均亮度」演算法,斷言模型已寫入;點擊「Run Inspection」,斷言:結果產生、恰好評估 1 個 ROI、整體判定標籤顯示 PASS 或 FAIL、匯出按鈕由停用變為啟用。

> 技術細節:`QTest::mouseMove` 不會合成「按鍵按住中」的移動事件,因此拖曳是直接建構 `QMouseEvent` 送入事件系統 — 這是 Qt UI 測試的標準做法,寫進輔助函式後可重複使用。

---

## 在 CI 中的實際表現

| 工作 | 平台 / 編譯器 | 內容 | 耗時(快取後) |
| --- | --- | --- | --- |
| `linux-fast` | Ubuntu / GCC | 建置 + 全部 6 套件 | 約 1 分 10 秒 |
| `windows` | Windows / MSVC | 建置 + 全部 6 套件 + windeployqt 打包 | 約 2 分 50 秒 |

- 兩個平台執行**同一套測試**,等於每次提交都做了一次跨編譯器驗證。
- 本機開發時以完全相同的指令執行:
  ```sh
  ctest --test-dir build/mac-debug --output-on-failure        # 全部測試
  ctest --test-dir build/mac-debug -R tst_algorithms          # 單一套件
  ```
- 全部 6 個套件在本機約 **5 秒內**跑完,適合每次修改後立即執行。

## 對正式產品的意義

這 6 個套件示範了三個可直接套用到 100+ 功能產品的測試層次:

1. **演算法層**(`tst_algorithms` 模式)— 用答案已知的黃金影像精確驗證每個檢測演算法,是回歸防護價值最高的一層。
2. **領域模型層**(`tst_inspectionplan`、`tst_scansimulator` 模式)— 驗證資料模型與裝置模擬介面,並以 `QAbstractItemModelTester` 之類的契約檢查器免費抓出整類缺陷。
3. **UI 冒煙層**(`tst_smoke_mainwindow` 模式)— 不求覆蓋每個畫面細節,而是把 QA 每次釋出前手動重測的關鍵流程自動化,每自動化一條流程就永久省下一份手動回歸工時。

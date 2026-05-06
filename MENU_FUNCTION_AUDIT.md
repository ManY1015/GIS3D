# MENU_FUNCTION_AUDIT

## 1. 加载单个模型
- 菜单文字：`加载单个模型`
- 命令 ID：`ID_LOAD_MODEL`
- 菜单位置�?  - [GIS3D.rc](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3D.rc:155)
- 消息映射位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:55)
- 处理函数位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:749)
- 核心代码逻辑�?  - 打开文件对话�?  - 调用 `CSceneObject::LoadModel`
  - `LoadModel` 内部调用 `osgDB::readNodeFile`
  - 成功�?`AddEvent(info)` 加入场景
  - `AddLayerToPane` 加入图层面板
- 实现状态判断：已实�?- 是否需要前置条件：不需�?- 如果点击没反应，可能原因�?  - 用户取消文件对话�?  - 选择文件读取失败
  - 界面过滤器只显示 `osgb`

## 2. 加载批量模型
- 菜单文字：`加载批量模型`
- 命令 ID：`ID_LOAD_BATCH_MODELS`
- 菜单位置�?  - [GIS3D.rc](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3D.rc:156)
- 消息映射位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:56)
- 处理函数位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:779)
- 核心代码逻辑�?  - 使用 `OFN_ALLOWMULTISELECT`
  - 循环读取多个文件
  - 每个文件调用 `LoadModel`
  - 成功后逐个 `AddEvent` �?`AddLayerToPane`
  - 最后弹出成�?失败统计
- 实现状态判断：已实�?- 是否需要前置条件：OSG viewer 已初始化
- 如果点击没反应，可能原因�?  - 场景尚未初始�?  - 用户取消选择
  - 文件格式筛选仍只显�?`osgb`

## 3. 批量模型总览
- 菜单文字：`批量模型总览`
- 命令 ID：`ID_EXP_BATCH_OVERVIEW`
- 菜单位置�?  - [GIS3D.rc](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3D.rc:157)
- 消息映射位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:59)
- 处理函数位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:930)
- 核心代码逻辑�?  - 调用 `EnsureExperimentFeatures`
  - 进入 `CExperimentFeatures::FocusBatchModels`
  - 扫描 root 中批量模型图�?  - 聚合包围盒并移动相机
- 实现状态判断：部分实现
- 是否需要前置条件：
  - 先批量加载模�?  - 图层名称要符合批量模型命名规�?- 如果点击没反应，可能原因�?  - 没有批量模型图层
  - 状态提示写�?`UpdateTerrainStatus`，但用户不一定看得到

## 4. 加载地形与影�?- 菜单文字：`加载地形与影像`
- 命令 ID：`ID_LOAD_TERRAIN`
- 菜单位置�?  - [GIS3D.rc](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3D.rc:159)
- 消息映射位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:54)
- 处理函数位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:691)
- 核心代码逻辑�?  - 选择 DEM
  - 可选选择影像
  - 调用 `CreateTerrain`
  - 成功�?`AddEvent(info)` 加场景，`AddLayerToPane("地形")`
- 实现状态判断：已实�?- 是否需要前置条件：需要真�?DEM；影像可�?- 如果点击没反应，可能原因�?  - 用户取消选择
  - DEM 创建失败

## 5. 保存视角书签
- 菜单文字：`保存视角书签`
- 命令 ID：`ID_EXP_BOOKMARK_SAVE`
- 消息映射位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:57)
- 处理函数位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:918)
- 核心代码逻辑�?  - 读取当前 camera view matrix
  - 保存�?`ViewBookmark` 内存结构
- 实现状态判断：已实�?- 是否需要前置条件：场景�?viewer 已就�?- 如果点击没反应，可能原因�?  - 成功提示只写�?`UpdateTerrainStatus`
  - 不会�?MessageBox

## 6. 恢复视角书签
- 菜单文字：`恢复视角书签`
- 命令 ID：`ID_EXP_BOOKMARK_RESTORE`
- 消息映射位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:58)
- 处理函数位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:926)
- 核心代码逻辑�?  - �?bookmark 有效，则应用保存�?eye/center/up
- 实现状态判断：已实�?- 是否需要前置条件：必须先保存过书签
- 如果点击没反应，可能原因�?  - 从未保存过书�?  - 失败信息同样只走 `UpdateTerrainStatus`

## 7. 对象包围盒与属性查�?- 菜单文字：`对象包围盒与属性查询`
- 命令 ID：`ID_EXP_OBJECT_INSPECT`
- 消息映射位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:61)
- 处理函数位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:950)
- 核心代码逻辑�?  - 菜单点击只切�?`m_bInspectorEnabled`
  - 实际查询发生�?`CExperimentFeatures::HandlePick`
  - 左键点击对象后弹窗并绘制包围�?- 实现状态判断：已实�?- 是否需要前置条件：
  - 先加载模型或地形
  - 菜单开启后再左键点击对�?- 如果点击没反应，可能原因�?  - 只点菜单没有继续点击对象
  - 没有已加载对�?  - 提示仍依赖状态文本接�?
## 8. 地形高程查询
- 菜单文字：`地形高程查询`
- 命令 ID：`ID_EXP_TERRAIN_QUERY`
- 消息映射位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:62)
- 处理函数位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:958)
- 核心代码逻辑�?  - 菜单点击只切�?`m_bTerrainQueryEnabled`
  - 真正查询�?`CExperimentFeatures::HandlePick`
  - 仅当左键点击到“地形图层”时才弹出高程信�?- 实现状态判断：已实�?- 是否需要前置条件：
  - 必须先加�?DEM 地形
  - 必须点击菜单后，再左键点击地形表�?- 如果点击没反应，可能原因�?  - 只点击菜单，没有再点地形
  - 尚未加载地形
  - 点到的不是地形层
  - 进入模式的提示写�?`UpdateTerrainStatus`，用户未必看得到

## 9. 线框检查模�?- 菜单文字：`线框检查模式`
- 命令 ID：`ID_EXP_WIREFRAME`
- 消息映射位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:63)
- 处理函数位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:966)
- 核心代码逻辑�?  - 遍历 root 下有名称的图�?  - �?`PolygonMode LINE/FILL` 做全局切换
- 实现状态判断：已实�?- 是否需要前置条件：场景里至少要有已加载图层
- 如果点击没反应，可能原因�?  - 当前无图�?  - 状态切换提示只写到 `UpdateTerrainStatus`

## 10. 导出场景截图
- 菜单文字：`导出场景截图`
- 命令 ID：`ID_EXP_SCREENSHOT`
- 消息映射位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:60)
- 处理函数位置�?  - [GIS3DView.cpp](D:/专业学习/三维GIS/实习�?GIS3D3/GIS3D3/GIS3DView.cpp:942)
- 核心代码逻辑�?  - 弹保存对话框
  - `CaptureClientToFile` 抓取当前客户�?  - 使用 `CImage::Save` 导出 `bmp/png`
- 实现状态判断：已实�?- 是否需要前置条件：窗口存在即可
- 如果点击没反应，可能原因�?  - 用户取消保存对话�?  - 保存失败但失败信息只写进状态文本接�?
---

## ���θ̲߳�ѯ���ܲ�����ƣ���ʵ��ͨ����
- ��ǰ״̬����ʵ��ͨ����
- ��ȷ������
  1. �ȼ��ص�����Ӱ��
  2. ��� ʵ�� -> ���θ̲߳�ѯ��
  3. ���������ѯģʽ��ʾ��
  4. ���������α��棻
  5. �����߳���Ϣ��
- ʵ������
  - �˵��ɴ�����ѯģʽ��
  - ���������α����ɵ����߳���Ϣ��
  - ��ǰ������ʾ���� Scene Elevation��
  - ��ֵ���������̣߳�����֤��ͬ��ԭʼ DEM ���Ը̡߳�
- ��ˣ������ܲ������ڡ�����û��Ӧ/����֤�������ǡ���ʵ������ʵ��ͨ������
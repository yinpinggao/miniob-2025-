#!/usr/bin/env python3
"""
手动执行文档摄入到 MiniOB
"""

import os
import sys

# 添加路径
sys.path.insert(0, '/root/lf-venv/lib/python3.12/site-packages')

from miniob_client import MiniOBClient, vector_to_string, escape_sql_string

# 初始化 Embeddings
try:
    from langchain_ollama import OllamaEmbeddings
    print("✓ Imported OllamaEmbeddings")
except ImportError:
    try:
        from langchain_community.embeddings import OllamaEmbeddings
        print("✓ Imported OllamaEmbeddings (community)")
    except ImportError:
        print("✗ Failed to import OllamaEmbeddings")
        sys.exit(1)

# 初始化 Document Loader
try:
    from langchain_community.document_loaders import DirectoryLoader, TextLoader
    from langchain_text_splitters import RecursiveCharacterTextSplitter
    print("✓ Imported Document Loaders")
except ImportError:
    print("✗ Failed to import Document Loaders")
    sys.exit(1)


def main():
    print("=" * 70)
    print("📚 MiniOB RAG 文档摄入脚本")
    print("=" * 70)
    
    # 配置
    HOST = '127.0.0.1'
    PORT = 6789
    TABLE_NAME = 'rag_documents'
    # 从环境变量获取文档路径，如果没有则使用默认路径
    DOC_PATH = '/root/source/miniob/zh-CN'
    
    print(f"\n📋 配置：")
    print(f"  - MiniOB: {HOST}:{PORT}")
    print(f"  - 表名: {TABLE_NAME}")
    print(f"  - 文档路径: {DOC_PATH}")
    
    # 1. 初始化 Embedding 模型
    print("\n" + "=" * 70)
    print("🔢 步骤 1: 初始化 Ollama Embedding 模型 (bge-m3)")
    print("=" * 70)
    
    try:
        embeddings = OllamaEmbeddings(
            base_url="http://localhost:11434",
            model="bge-m3"
        )
        # 测试 embedding
        test_embedding = embeddings.embed_query("测试")
        print(f"✓ Embedding 模型加载成功")
        print(f"  - 维度: {len(test_embedding)}")
    except Exception as e:
        print(f"✗ Embedding 模型加载失败: {e}")
        return
    
    # 2. 加载文档
    print("\n" + "=" * 70)
    print("📂 步骤 2: 加载文档")
    print("=" * 70)
    
    try:
        loader = DirectoryLoader(
            DOC_PATH,
            glob="**/*.md",
            loader_cls=TextLoader,
            loader_kwargs={'encoding': 'utf-8'},
            show_progress=True
        )
        documents = loader.load()
        print(f"✓ 加载了 {len(documents)} 个文档")
    except Exception as e:
        print(f"✗ 文档加载失败: {e}")
        return
    
    # 3. 分割文档
    print("\n" + "=" * 70)
    print("✂️ 步骤 3: 分割文档")
    print("=" * 70)
    
    try:
        text_splitter = RecursiveCharacterTextSplitter(
            chunk_size=500,
            chunk_overlap=50
        )
        chunks = text_splitter.split_documents(documents)
        print(f"✓ 分割成 {len(chunks)} 个文本块")
    except Exception as e:
        print(f"✗ 文档分割失败: {e}")
        return
    
    # 处理合理数量的数据
    MAX_CHUNKS = 200 # 用于测评的数据量
    if len(chunks) > MAX_CHUNKS:
        print(f"\n⚠️  限制处理前 {MAX_CHUNKS} 个文本块（共 {len(chunks)} 个）")
        chunks = chunks[:MAX_CHUNKS]
    else:
        print(f"\n✅ 将处理全部 {len(chunks)} 个文本块")
    
    print(f"⏱️  预计耗时: {len(chunks) * 0.6 / 60:.1f} 分钟")
    
    # 4. 连接 MiniOB
    print("\n" + "=" * 70)
    print("🔌 步骤 4: 连接 MiniOB")
    print("=" * 70)
    
    client = MiniOBClient(host=HOST, port=PORT)
    if not client.connect():
        print("✗ 无法连接到 MiniOB")
        return
    
    # 5. 创建表
    print("\n" + "=" * 70)
    print("🏗️ 步骤 5: 创建表")
    print("=" * 70)
    
    try:
        create_table_sql = f"""
        CREATE TABLE {TABLE_NAME} (
            id INT,
            content TEXT,
            embedding VECTOR(1024)
        );
        """
        result = client.execute(create_table_sql)
        print(f"✓ 表 {TABLE_NAME} 创建成功")
    except Exception as e:
        error_msg = str(e)
        if 'exist' in error_msg.lower() or 'duplicate' in error_msg.lower():
            print(f"ℹ️  表 {TABLE_NAME} 已存在")
        else:
            print(f"⚠️  创建表时出错: {e}")
    
    # 6. 插入数据
    print("\n" + "=" * 70)
    print("📥 步骤 6: 插入文档数据")
    print("=" * 70)
    
    success_count = 0
    
    try:
        # 获取起始 ID
        try:
            max_id_sql = f"SELECT MAX(id) FROM {TABLE_NAME};"
            rows = client.execute_query(max_id_sql)
            start_id = 1
            # rows[0] 是表头，rows[1] 才是数据
            if rows and len(rows) > 1 and rows[1] and rows[1][0] and rows[1][0].strip():
                try:
                    max_id = int(rows[1][0].strip())
                    start_id = max_id + 1
                    print(f"📊 表中已有 {max_id} 条记录，起始 ID: {start_id}")
                except Exception as e:
                    start_id = 1
                    print(f"📊 解析 MAX(id) 失败，从 ID 1 开始: {e}")
            else:
                print(f"📊 表是空的，起始 ID: 1")
        except Exception as e:
            start_id = 1
            print(f"📊 查询 MAX(id) 失败，从 ID 1 开始: {e}")
        
        # 插入文档
        for i, chunk in enumerate(chunks):
            doc_id = start_id + i
            print(f"\n处理文档 {doc_id}/{start_id + len(chunks) - 1}...")
            
            try:
                # 生成 embedding
                print(f"  🔢 生成 embedding...")
                embedding = embeddings.embed_query(chunk.page_content)
                embedding_str = vector_to_string(embedding)
                print(f"  ✓ Embedding 维度: {len(embedding)}")
                
                # 准备 content
                content = chunk.page_content[:1000] if len(chunk.page_content) > 1000 else chunk.page_content
                content_escaped = escape_sql_string(content)
                
                # 插入
                insert_sql = f"""
                INSERT INTO {TABLE_NAME} VALUES (
                    {doc_id},
                    '{content_escaped}',
                    {embedding_str}
                );
                """
                
                print(f"  💾 插入数据...")
                result = client.execute(insert_sql)
                
                if 'FAILURE' not in result:
                    success_count += 1
                    print(f"  ✅ 文档 {doc_id} 插入成功")
                else:
                    print(f"  ✗ 插入失败: {result}")
                    
            except Exception as e:
                print(f"  ✗ 错误: {e}")
                continue
        
        print("\n" + "=" * 70)
        print(f"✅ 成功插入 {success_count}/{len(chunks)} 个文档")
        print("=" * 70)
        
    except Exception as e:
        print(f"✗ 插入数据时出错: {e}")
        import traceback
        traceback.print_exc()
    
    finally:
        client.close()
    
    # 7. 验证
    print("\n" + "=" * 70)
    print("🔍 步骤 7: 验证数据")
    print("=" * 70)
    
    client2 = MiniOBClient(host=HOST, port=PORT)
    client2.connect()
    
    try:
        count_result = client2.execute(f"SELECT COUNT(*) FROM {TABLE_NAME};")
        print(f"📊 表中记录数：\n{count_result}")
        
        sample_result = client2.execute(f"SELECT id FROM {TABLE_NAME} LIMIT 3;")
        print(f"\n📄 前 3 条记录 ID：\n{sample_result}")
    except Exception as e:
        print(f"✗ 验证失败: {e}")
    finally:
        client2.close()
    
    print("\n" + "=" * 70)
    print("🎉 文档摄入完成！")
    print("=" * 70)


if __name__ == "__main__":
    main()

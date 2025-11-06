#!/usr/bin/env python3
import sys
sys.path.insert(0, '/root/lf-venv/lib/python3.12/site-packages')

from miniob_client import MiniOBClient, vector_to_string
from langchain_ollama import OllamaEmbeddings
from langchain_ollama import ChatOllama

print("=" * 80)
print("🧪 本地 RAG 完整流程测试")
print("=" * 80)

# 1. 连接 MiniOB
print("\n【1/5】连接 MiniOB...")
client = MiniOBClient(host='127.0.0.1', port=6789)
if not client.connect():
    print("   ✗ 连接失败")
    sys.exit(1)
print("   ✓ 连接成功")

# 2. 检查数据
print("\n【2/5】检查数据...")
count_sql = "SELECT COUNT(*) FROM rag_documents;"
result = client.execute(count_sql)  # 使用 execute，不是 execute_query
print(f"   原始结果: {result}")

# 解析 COUNT(*) 结果
lines = result.strip().split('\n')
if len(lines) >= 2:
    count_line = lines[1].strip()  # 第二行是数据
    doc_count = int(count_line) if count_line else 0
    print(f"   ✓ 数据库中有 {doc_count} 条文档")
else:
    print(f"   ✗ 无法解析文档数量")
    client.close()
    sys.exit(1)

# 3. 用户输入问题
query = input("\n【3/5】请输入你的问题（直接回车使用默认问题）: ").strip()
if not query:
    query = "什么是 OceanBase"
print(f"   ✓ 查询: '{query}'")

# 4. 生成查询向量并检索
print("\n【4/5】检索相关文档...")
embeddings = OllamaEmbeddings(model="bge-m3", base_url="http://localhost:11434")
query_embedding = embeddings.embed_query(query)
embedding_str = vector_to_string(query_embedding)

search_sql = f"SELECT id, content FROM rag_documents ORDER BY L2_DISTANCE(embedding, {embedding_str}) LIMIT 4;"
result = client.execute(search_sql)

# 调试：打印原始结果
print(f"   原始查询结果:\n{result[:500]}\n")

# 解析结果
lines = result.strip().split('\n')
context_docs = []
print(f"   总共 {len(lines)} 行，解析如下：")
for i, line in enumerate(lines[1:]):  # 跳过表头
    line = line.strip()
    if not line or line.startswith('---'):
        continue
    parts = [p.strip() for p in line.split('|')]
    if len(parts) >= 2:
        doc_id, content = parts[0], parts[1]
        context_docs.append(content)
        print(f"   {i+1}. [Doc {doc_id}] {content[:60]}...")

if not context_docs:
    print("   ✗ 未检索到相关文档")
    client.close()
    sys.exit(1)

context = "\n\n".join(context_docs)
print(f"   ✓ 检索到 {len(context_docs)} 个相关文档")

# 5. 生成回答
print("\n【5/5】生成回答...")
llm = ChatOllama(model="qwen2.5:7b", base_url="http://localhost:11434")

prompt = f"""根据以下参考信息回答问题。

参考资料：
{context}

问题：{query}

请用中文回答，并基于参考资料提供准确的信息。"""

print("\n" + "=" * 80)
print("💬 回答：")
print("-" * 80)

response = llm.invoke(prompt)
answer = response.content
print(answer)

print("-" * 80 + "\n")
print("✅ 测试完成！")
print("=" * 80)
client.close()
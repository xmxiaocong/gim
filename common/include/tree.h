#ifndef _ZK_TREE_H_
#define _ZK_TREE_H_

#include <string>
#include <vector>
#include <set>
#include <map>
#include <json/json.h>
using namespace std;

namespace ef
{
	class Tree;
	class TreeNode
	{
		friend class Tree;
	private:
		TreeNode(const std::string& s);
		~TreeNode();
	public:
		const std::string& getData() const;
		void setData(const std::string& d);
		const std::string& getId();
	private:
		TreeNode* addChild(const std::string& s);
		void deleteChild(const std::string& s);
		void removeChild(const std::string& s);
		TreeNode* getNode(const std::string& id);

		void getChildren(std::vector<std::string>& v);
	private:
		std::string data_;
		std::string id_;
		std::vector<TreeNode*> nodes_;
	};

	class Tree
	{
	public:
		Tree();
		~Tree();
		void getChildren(const std::string& path, std::vector<std::string>& v);

		void delNode(const std::string& path);

		TreeNode* getNode(const std::string& path);
		TreeNode* addNode(const std::string& path);
	public:
		static std::string genPathName(const std::string& path, const std::string& id);
		static int split(const std::string& str, std::vector<std::string>& ret_);
		void getTreeJson(const std::string& path, Json::Value& v);

		static bool isSubpath(const std::string& path, const std::string& subpath, unsigned int depth);
		static std::string getParentPath(const std::string& path);
	private:
		TreeNode root;
	};
}

#endif //_ZK_TREE_H_

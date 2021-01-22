#ifndef NONCOPYABLE_H_
#define NONCOPYABLE_H_

//删除拷贝构造和拷贝赋值
class NonCopyAble {
 public:
    NonCopyAble(const NonCopyAble&) = delete;
    NonCopyAble& operator=(const NonCopyAble&) = delete;
    NonCopyAble(NonCopyAble&&) = default;
    NonCopyAble& (NonCopyAble&&) = default;
 protected:
    NonCopyAble() = default;
    ~NonCopyAble() = default;
};

#endif

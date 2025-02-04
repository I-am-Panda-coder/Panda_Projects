#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <stdint.h>
#include <functional>
#include "F4SE/F4SE.h"
#include "RE/Fallout.h"
#include "NAFicator/XML/utils.h"

namespace logger = F4SE::log;
using namespace std::string_literals;

class Archive;
class Form;
class IdleForm;

template <class T>
std::shared_ptr<T> make_form(const std::string& formId, const std::string& source, const std::string& optional_value = ""s)
{
#pragma warning(push)
#pragma warning(disable: 4840)
	return std::shared_ptr<T>(new T(formId, source, optional_value));
#pragma warning(pop)
}

//template <class T, class ...Args>
//std::shared_ptr<Form> make_form(const std::string&, const std::string&, Args... args);

std::shared_ptr<Form> extract(std::shared_ptr<Form>);
std::shared_ptr<Form> put(std::shared_ptr<Form>);

class Form
{
	friend class Archive;
	friend std::shared_ptr<Form> make_form<Form>(const std::string&, const std::string&, const std::string&);

protected:
	std::string m_source;
	std::string m_form;
	bool hasValue;
	

	uint32_t get_uint() const;
	void normalize();

	Form() :
		m_source(""), m_form(""), hasValue(false) {}

	Form(const std::string& form, const std::string& source...) :
		m_form(form), m_source(source)
	{
		normalize();
		hasValue = true;
	}

public:

	virtual ~Form() 
	{
		hasValue = false;
	}

	RE::TESForm* get() const;
	bool operator==(const Form& f) const;
	bool has_value() { return hasValue; }
	virtual std::string make_archive_string() const;
};

class IdleForm : public Form
{
	std::string m_hkx;
	friend class Archive;

	void normalize();

	IdleForm() :
		Form(), m_hkx("") {}

	IdleForm(const Form& f) :
		Form(f), m_hkx("") {}

	IdleForm(const Form& f, const std::string& hkx) :
		Form(f), m_hkx(hkx) { utils::remove_spaces_from_sides(m_hkx); }

	IdleForm(const std::string& form, const std::string& source, const std::string& hkx) :
		Form(form, source), m_hkx(hkx) { utils::remove_spaces_from_sides(m_hkx); }

	friend std::shared_ptr<IdleForm> make_form<IdleForm>(const std::string&, const std::string&, const std::string&);

public:

	~IdleForm() override
	{
		hasValue = false;
	}

	std::string make_archive_string() const override;
	std::string& hkx() { return m_hkx; }
};

class Archive
{
	enum flag
	{
		read = std::ios::in,
		write = std::ios::out | std::ios::app,
		read_write = std::ios::out | std::ios::in | std::ios::app
	};

	static inline Archive* archive;

	const char* path_to_archive = "\\Data\\NAFicator\\archive.arc";
	std::fstream file;
	std::mutex lock;

	std::queue<std::pair<std::function<std::shared_ptr<Form>(std::shared_ptr<Form>)>, std::shared_ptr<Form>>> queue;
	
	std::shared_ptr<Form> find_and_apply(const std::string& form, const std::string& source, const std::function<std::shared_ptr<Form>(const std::string&)>& apply);
	std::shared_ptr<Form> find_and_apply(const std::shared_ptr<Form> form, const std::function<std::shared_ptr<Form>(const std::string&)>& apply);
	std::shared_ptr<Form> get_form(const std::string& str, const std::string& form, const std::string& source) const;
	std::shared_ptr<Form> get_form(const std::string& str, const std::shared_ptr<Form> form) const;
	Archive(){};

	friend std::shared_ptr<Form> extract(std::shared_ptr<Form>);
	friend std::shared_ptr<Form> put(std::shared_ptr<Form>);

public:

	static Archive* get_singleton()
	{
		if (!Archive::archive)
			Archive::archive = new Archive();
		return Archive::archive;
	}

	Archive(const Archive&) = delete;
	Archive operator=(Archive&) = delete;

	inline std::string print_flag(Archive::flag f)
	{
		switch (f)
		{
		case read:
			return "read"s;
		case write:
			return "write"s;
		case read_write:
			return "read_write"s;
		default:
			return ""s;
		}
	}
};

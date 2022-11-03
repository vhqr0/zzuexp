from django import forms
from django.contrib.auth import authenticate, login

from .models import UserInterface, VerifyRecord, Avatar, Category, Page


class LoginForm(forms.Form):
    username = forms.CharField(max_length=150,
                               help_text='Please enter your username.')
    password = forms.CharField(max_length=128,
                               widget=forms.PasswordInput,
                               help_text='Please enter your password.')

    def clean(self):
        username = self.cleaned_data['username']
        password = self.cleaned_data['password']
        self.user = authenticate(username=username, password=password)
        if self.user is None or not self.user.is_active:
            raise forms.ValidationError('Invalid username or password!')
        return super().clean()

    def login(self, request):
        login(request, self.user)


class SigninForm(forms.Form):
    username = forms.CharField(max_length=150,
                               required=False,
                               help_text='Please enter your username.')
    password = forms.CharField(max_length=128,
                               widget=forms.PasswordInput,
                               help_text='Please enter your password.')
    email = forms.EmailField(help_text='Please enter your email.')
    verify_type = forms.ChoiceField(choices=(('signin', 'Signin'),
                                             ('changepwd', 'Change Password')),
                                    initial='signin',
                                    help_text='Please select verify type.')

    def clean(self):
        username = self.cleaned_data['username']
        email = self.cleaned_data['email']
        verify_type = self.cleaned_data['verify_type']
        if verify_type == 'signin':
            if not username:
                raise forms.ValidationError('Username cannot be blank!')
            if UserInterface.username_or_email_count(username, email) != 0:
                raise forms.ValidationError(
                    'Username or Email is already exists!')
        else:
            if UserInterface.email_count(email) == 0:
                raise forms.ValidationError('Email is not exists!')
        return super().clean()

    def save(self):
        record = VerifyRecord(username=self.cleaned_data['username'],
                              password=self.cleaned_data['password'],
                              email=self.cleaned_data['email'],
                              verify_type=self.cleaned_data['verify_type'])
        record.save()
        return record.pk


class SigninVerifyForm(forms.Form):
    verify_code = forms.UUIDField(
        help_text='Please enter verify code you received.')

    def clean(self):
        self.record = VerifyRecord.objects.get(pk=self.record_pk)
        if not self.record.is_valid(self.cleaned_data['verify_code']):
            raise forms.ValidationError('Invalid verify code!')
        return super().clean()

    def signin(self):
        return self.record.signin()


class AvatarUploadForm(forms.Form):
    avatar = forms.ImageField(help_text='Please upload an image < 4KB.')

    def clean(self):
        if self.cleaned_data['avatar'].size > 4096:
            raise forms.ValidationError('Image too big!')
        return super().clean()

    def save(self, request):
        Avatar.set(request.user, self.cleaned_data['avatar'])


class CategoryAddForm(forms.Form):
    name = forms.CharField(max_length=128,
                           help_text='Please enter the category name.')

    def clean(self):
        if Category.objects.filter(
                name=self.cleaned_data['name']).count() != 0:
            raise forms.ValidationError(
                f'Category {self.cleaned_data["name"]} is already exists!')
        return super().clean()

    def save(self):
        category = Category(name=self.cleaned_data['name'])
        category.save()


class PageAddForm(forms.Form):
    category = forms.CharField(
        max_length=128, help_text='Please enter the category of the page.')
    title = forms.CharField(max_length=128,
                            help_text='Please enter the title of the page.')
    url = forms.URLField(help_text='Please enter the URL of the page.')

    def clean(self):
        categories = Category.objects.filter(
            name=self.cleaned_data['category'])
        if categories.count() == 0:
            raise forms.ValidationError(
                f'category {self.cleaned_data["category"]} is not exists!')
        category = categories[0]
        pages = Page.objects.filter(category=category,
                                    title=self.cleaned_data['title'])
        if pages.count() != 0:
            raise forms.ValidationError(
                f'page {self.cleaned_data["title"]} is already exists!')
        return super().clean()

    def save(self):
        category = Category.objects.get(name=self.cleaned_data['category'])
        title = self.cleaned_data['title']
        url = self.cleaned_data['url']
        page = Page(category=category, title=title, url=url)
        page.save()
